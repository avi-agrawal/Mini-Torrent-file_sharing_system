#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>
#include <bits/stdc++.h>
#include <arpa/inet.h>
using namespace std;

//global
int buffer_size = 2048;
int max_peer_count = 5;
map<string,string> database;
vector<string> curr_logins;
map<string, vector<string>> groups;     //map<group_id,vector<user_id>>
map<string, vector<string>> requests;

//map<group_id,vector<pait<file_name,owner>>>
map<string, vector<pair<string,string>>> files;

//map< file_name, pair<file_size,vector<pair<ip,port>>> >
map< string, pair<string,vector<pair<string,string>>> > file_info;
// map<pair<string,string>, vector<pair<string,string>>> file_info;

map <string,string> file_path_map;   //filename,filepath

//functions
vector<string> split(string str,char ch);
string send_to_peer(int socket, string msg);
string read_from_peer(int socket);
void create_user(string user_id, string password, int newsock_fd);
void login(string user_id, string password, int newsock_fd);
void logout(string user_id, int newsock_fd);
void create_group(string group_id,string curr_user_login ,int newsock_fd);
void join_group(string group_id, string curr_user_login, int newsock_fd);
void leave_group(string group_id, string curr_user_login, int newsock_fd);
void list_requests(string group_id, string curr_user_login, int newsock_fd);
void accept_request(string group_id, string user_id, string curr_user_login, int newsock_fd);
void list_groups(int newsock_fd);
void upload_file(string file_path, string group_id, string curr_user_login, int newsock_fd);
void list_files(string group_id, int newsock_fd);
void *check_input(void *thread_info_ip);


class info{
    public:
        int port;
        int sock_fd;
        int newsock_fd;
};

//function to return vector of space seperated string after splitting
vector<string> split(string str,char ch)
{
    vector<string> split_str;
    stringstream ss(str);
    string word;
    while(getline(ss,word,ch)){
        split_str.push_back(word);
    }
    return split_str;
}

//function for sent msg to peer
string send_to_peer(int socket, string msg)
{
    int check = send(socket,msg.c_str(),msg.size(),0);
    if(check < 0){
        perror("Error in sending msg to client");
    }
    msg = "";
    return msg;
}

//function to read msg from peer
string read_from_peer(int socket)
{
    char buffer[buffer_size] = {0};
    int check = read(socket,buffer,sizeof(buffer));
    if(check < 0){
        perror("Error in sending msg to client");
    }
    string msg = string(buffer);
    bzero(buffer,sizeof(buffer));
    return msg;
}

//create_user funcition
void create_user(string user_id, string password, int newsock_fd)
{
    if((database.find(user_id)) != database.end()){
        string msg = "User already exist";
        cout<<msg<<endl;
        msg = send_to_peer(newsock_fd,msg);
    }
    //if user not exist
    else{
        // database[user_id]=password;
        database.insert({user_id,password});
        string msg = "User created";
        cout<<"User: "<<user_id<<" ,created"<<endl;
        msg = send_to_peer(newsock_fd,msg);
        
        
    }
    // close(newsock_fd);
}

//login function
void login(string user_id, string password, int newsock_fd)
{
    //if user not created
    if((database.find(user_id))==database.end()){
        string msg = user_id + " user_id not created";
        cout<<msg<<endl;
        msg = send_to_peer(newsock_fd,msg);
    }

    else{
        //checking password correct or not
        if(database[user_id]==password){
            //if user already logged in into any peer
            if(find(curr_logins.begin(),curr_logins.end(),user_id) == curr_logins.end()){
                curr_logins.push_back(user_id);
                cout<<"User not active"<<endl;
            }
            string msg = "Logged into the user";
            cout<<"Logged into the user: "<<user_id<<endl;
            msg = send_to_peer(newsock_fd,msg);
            cout<<"User now active";
        }
        else{
            //if passwrod incorrect
            string msg = "Password incorrect";
            cout<<msg<<endl;
            msg = send_to_peer(newsock_fd,msg);
        }
    }

}

//logout
void logout(string user_id, int newsock_fd)
{
    //if user already logged out or input user not exist
    if(find(curr_logins.begin(),curr_logins.end(),user_id) == curr_logins.end()){
        string msg = "User already logged out or user doesn't exist";
        cout<<msg<<endl;
        msg = send_to_peer(newsock_fd, msg);
    }
    else{
        curr_logins.erase(find(curr_logins.begin(),curr_logins.end(),user_id));
        string msg = "User id logged out";
        cout<<msg<<endl;
        msg = send_to_peer(newsock_fd,msg);
    }
}

//create_group function
void create_group(string group_id,string curr_user_login ,int newsock_fd)
{
    cout<<"user logged in: "<<curr_user_login<<endl;
    //if group_id already exist
    if(groups.find(group_id) != groups.end()){

        string msg = "Group_id already exist, owner: " + groups[group_id][0];
        cout<<msg<<endl;
        msg = send_to_peer(newsock_fd,msg);
    }
    else{
        cout<<"creating group..."<<endl;
        // groups.insert(pair<string,vector<string>>(group_id,{curr_user_login}));
        // groups[group_id].push_back(curr_user_login);
        groups[group_id];
        groups[group_id].push_back(curr_user_login);
        string msg = "Group created with owner as: ";
        msg = msg  + groups[group_id][0];
        cout<<"Group: "<<group_id<<" created"<<endl;
        msg = send_to_peer(newsock_fd,msg);
    }
}

//join group
void join_group(string group_id, string curr_user_login, int newsock_fd)
{
    cout<<"User requesting: "<<curr_user_login<<endl;
    cout<<"Group requesting to join: "<<group_id<<endl;

    //group doesnt exist
    if(groups.find(group_id) == groups.end()){

        string msg = "Group: " + group_id + " doesn't exist";
        cout<<msg<<endl;
        msg = send_to_peer(newsock_fd,msg);
    }
    //group exist
    else{
        //user already in the group
        if(find(groups[group_id].begin(),groups[group_id].end(),curr_user_login) != groups[group_id].end()){
            string msg = "User: " + curr_user_login + " already in the group: " + group_id;
            cout<<msg<<endl;
            msg = send_to_peer(newsock_fd,msg);
        }
        //user not in the group
        else{
            //if already a request exist of that group id
            if(requests.find(group_id) != groups.end()){
                //if already a pending request exist of the same user
                if(find(requests[group_id].begin(),requests[group_id].end(),curr_user_login) != requests[group_id].end()){
                    string msg = "Request for User: " + curr_user_login + " is pending";
                    cout<<msg<<endl;
                    msg = send_to_peer(newsock_fd,msg);
                }
                else{
                    requests[group_id];
                    requests[group_id].push_back(curr_user_login);
                    string msg = "Request sent";
                    cout<<msg<<endl;
                    msg = send_to_peer(newsock_fd,msg);
                }
            }
        }
    }
}

//leave group
void leave_group(string group_id, string curr_user_login, int newsock_fd)
{
    cout<<"Group to leave: "<<group_id<<endl;
    cout<<"User to leave: "<<curr_user_login<<endl;

    //if group doesn't exist
    if(groups.find(group_id) == groups.end()){
        string msg = "Group: " + group_id + " doesn't exist";
        cout<<msg<<endl;
        msg = send_to_peer(newsock_fd,msg);
    }
    //if group exist
    else{
        //if user is not the member of group
        if(find(groups[group_id].begin(),groups[group_id].end(),curr_user_login) == groups[group_id].end()){
            string msg = "User: " + curr_user_login + " is not the member of the group";
            cout<<msg<<endl;
            msg = send_to_peer(newsock_fd,msg);
        }
        //if user is member of group
        else{
            //if user is the owner of the group, then cannot leave grp
            if(groups[group_id][0]==curr_user_login){
                string msg = "User: " + curr_user_login + ", You are the owner of the grp, so cannot leave group";
                cout<<msg<<endl;
                msg = send_to_peer(newsock_fd,msg);
            }
            else{
                //user is not the owner
                groups[group_id].erase(find(groups[group_id].begin(),groups[group_id].end(),curr_user_login));
                string msg = "User: " + curr_user_login + " left the group: " + group_id;
                cout<<msg<<endl;
                msg = send_to_peer(newsock_fd,msg);
            }
        }
    }
}

//list requests function
void list_requests(string group_id, string curr_user_login, int newsock_fd)
{
    cout<<"Group requests of: "<<group_id<<endl;
    cout<<"User requesting: "<<curr_user_login<<endl;

    //if group doesn't exist
    if(groups.find(group_id) == groups.end()){
        string msg = "Group: " + group_id + " doesn't exist";
        cout<<msg<<endl;
        msg = send_to_peer(newsock_fd,msg);
    }

    else{
        //if user requesting for the list is not the owner of the grp
        if(groups[group_id][0]!=curr_user_login){
            string msg = "You cannot request pending lists, User: " + curr_user_login + " is not the owner of the group";
            cout<<msg<<endl;
            msg = send_to_peer(newsock_fd,msg);
        }
        //if the user is the owner
        else{
            //if no pending request exist
            if(requests.find(group_id) == requests.end()){
                string msg = "No pending requests exist";
                cout<<msg<<endl;
                msg = send_to_peer(newsock_fd,msg);
            }
            else{
                //if request exist
                string msg = "Pending Requests: \n";
                for(int i=0;i<requests[group_id].size();i++){
                    msg = msg + requests[group_id][i] + "\n";
                }
                cout<<msg<<endl;
                msg = send_to_peer(newsock_fd,msg);
            }
        }
    }
}

//accept request
void accept_request(string group_id, string user_id, string curr_user_login, int newsock_fd)
{
    cout<<"Group for accept request: "<<group_id<<endl;
    cout<<"User/Owner requesting: "<<curr_user_login<<endl;
    cout<<"Accept request of User: "<<user_id<<endl;

    //if group doesn't exist
    if(groups.find(group_id) == groups.end()){
        string msg = "Group: " + group_id + " doesn't exist";
        cout<<msg<<endl;
        msg = send_to_peer(newsock_fd,msg);
    }
    //if group exist
    else{
        //if user accepting request is not the owner of the grp
        if(groups[group_id][0]!=curr_user_login){
            string msg = "You cannot accept requests, User: " + curr_user_login + " is not the owner of the group";
            cout<<msg<<endl;
            msg = send_to_peer(newsock_fd,msg);
        }
        //if the user is the owner
        else{
            //if no pending request exist of the group
            if(requests.find(group_id) == requests.end()){
                string msg = "No pending request exist of the group: " + group_id;
                cout<<msg<<endl;
                msg = send_to_peer(newsock_fd,msg);
            }
            //if pending request exist
            else{
                //if user if already the member of the group
                if(find(groups[group_id].begin(),groups[group_id].end(),user_id) != groups[group_id].end()){
                    string msg = "User: " + user_id + " is already the member of the group";
                    cout<<msg<<endl;
                    msg = send_to_peer(newsock_fd,msg);
                }
                //if user is not already in the group
                else{
                    requests[group_id].erase(find(requests[group_id].begin(),requests[group_id].end(),user_id));
                    groups[group_id].push_back(user_id);
                    string msg = "Owner: " + curr_user_login + " accepted the request, User: " + user_id + " joined the grp";
                    cout<<msg<<endl;
                    msg = send_to_peer(newsock_fd,msg);
                    
                    //if pending request vector for the group is empty, then remove the key from map
                    if(requests[group_id].empty()){
                        requests.erase(group_id);
                    }
                }
            }
        }
    }
}

//list groups
void list_groups(int newsock_fd)
{
    //if groups map is empty, i.e. no groups exist
    if(groups.empty()){
        string msg = "No group exist";
        cout<<msg<<endl;
        msg = send_to_peer(newsock_fd,msg);
    }
    //if groups exist
    else{
        string msg = "Current groups in the server: \n";
        map<string,vector<string>> :: iterator itr;
        for(itr=groups.begin();itr!=groups.end();itr++){
            msg = msg + string(itr->first) + " -> Owner: " + groups[itr->first][0] + "\n";
        }
        cout<<msg<<endl;
        msg = send_to_peer(newsock_fd,msg);
    }
}

//upload files
void upload_file(string file_name, string group_id, string curr_user_login, string file_size, string ip_client, string port_client, string file_path, int newsock_fd)
{
    string res = "0";
    cout<<"File uploading in Group: "<<group_id<<endl;
    cout<<"File_name: "<<file_name<<endl;
    cout<<"File_size: "<<file_size<<endl;
    cout<<"Owner: "<<curr_user_login<<endl;

    //if group not created yet/group doesn't exist
    if(groups.find(group_id) == groups.end()){
        string msg = "Group: " + group_id + " doesn't exist";
        cout<<msg<<endl;
        msg = send_to_peer(newsock_fd,msg);
    }
    //if group exist in group map
    else{
        //if user is not the member of the group
        if(find(groups[group_id].begin(),groups[group_id].end(),curr_user_login) == groups[group_id].end()){
            string msg = "User: " + curr_user_login + " is not the member of the group";
            cout<<msg<<endl;
            msg = send_to_peer(newsock_fd,msg);
            return;
        }

        //if group doesn't exist in the files map/no file uploaded in the group yet
        if(files.find(group_id) == files.end()){
            string msg = "This is 1st upload for the group.";
            cout<<msg<<endl;
            res = "1";
            msg = send_to_peer(newsock_fd,res);
            files[group_id];
            files[group_id].push_back(make_pair(file_name,curr_user_login));
            file_info[file_name];
            file_info[file_name].first = file_size;
            (file_info[file_name].second).push_back(make_pair(ip_client,port_client));
            file_path_map[file_name];
            file_path_map[file_name] = file_path;
        }
        //if group exist in files map
        else{
            //if file_name already exist in the group
            int exist = 0;
            vector<pair<string,string>> :: iterator itr;
            for(itr=files[group_id].begin();itr!=files[group_id].end();itr++){
                if(itr->first == file_name){
                    exist = 1;
                    break;
                }
            }
            if(exist){
                string msg = "File with name: " + file_name + " already uploaded in the group";
                cout<<msg<<endl;
                msg = send_to_peer(newsock_fd,msg);
            }
            //if file not in group, then upload
            else{
                cout<<"Uploading file..."<<endl;
                files[group_id].push_back(make_pair(file_name,curr_user_login));
                file_info[file_name];
                file_info[file_name].first = file_size;
                (file_info[file_name].second).push_back(make_pair(ip_client,port_client));
                file_path_map[file_name];
                file_path_map[file_name] = file_path;
                string msg = "File Uploaded to the group:" + group_id;
                cout<<msg<<endl;
                res = "2";
                msg = send_to_peer(newsock_fd,res);
            }
        }
    }
    res = "0";
}

//list files in a group
void list_files(string group_id, int newsock_fd)
{
    //if files map is empty, i.e. no files exist
    if(files.empty()){
        string msg = "No file exist";
        cout<<msg<<endl;
        msg = send_to_peer(newsock_fd,msg);
    }
    //if files map exist
    else{
        //if input group id doesn't exist in groups map
        if(groups.find(group_id)==groups.end()){
            string msg = "No such group exist";
            cout<<msg<<endl;
            msg = send_to_peer(newsock_fd,msg);
        }
        //if input group id exist in groups map
        else{
            //if no file uploaded for the group/no group id exist in the files map
            if(files.find(group_id) == files.end()){
                string msg = "No file has been uploaded in the group: " + group_id;
                cout<<msg<<endl;
                msg = send_to_peer(newsock_fd,msg);
            }
            //if files are uploaded for the group
            else{
                //printing files
                string msg = "Files in the group: " + group_id + " :-\n";
                vector<pair<string,string>> :: iterator itr;
                for(itr=files[group_id].begin();itr!=files[group_id].end();itr++){
                    msg = msg + string(itr->first) + " -> Owner: " + string(itr->second) + "\n";
                }
                cout<<msg<<endl;
                msg = send_to_peer(newsock_fd,msg);
            }
        }
    }
}

//download file
void download_file(string group_id, string file_name, int newsock_fd)
{
    string res = "0";
    //if files map is empty, i.e. no files exist
    if(files.empty()){
        string msg = "No file exist";
        cout<<msg<<endl;
        msg = send_to_peer(newsock_fd,msg);
    }
    //if files map exist
    else{
        //if input group id doesn't exist in groups map
        if(groups.find(group_id)==groups.end()){
            string msg = "No such group exist";
            cout<<msg<<endl;
            msg = send_to_peer(newsock_fd,msg);
        }
        //if input group id exist in groups map
        else{
            //if no file uploaded for the group/no group id exist in the files map
            if(files.find(group_id) == files.end()){
                string msg = "No such file has been uploaded in the group: " + group_id;
                cout<<msg<<endl;
                msg = send_to_peer(newsock_fd,msg);
            }
            //file exist
            else{
                string ip_port_of_client = (file_info[file_name].second)[0].first + ":" + (file_info[file_name].second)[0].second;
                res = "1";
                // res = send_to_peer(newsock_fd,res);

                string msg = res + ":" + ip_port_of_client + ":" + file_path_map[file_name];
                cout<<msg<<endl;
                msg = send_to_peer(newsock_fd,msg);
                ip_port_of_client = "";
            }
        }
    }
}

//check_input in int main(while)
void *check_input(void *thread_info_ip)
{
    info *thread_info = (info *)thread_info_ip;
    while(1){

        // string cmd_from_tracker = "";
        // getline(cin,cmd_from_tracker);
        // if(cmd_from_tracker == "quit"){
        //     break;
        // }

        cout<<endl;
        cout<<"Client: ";
        // char cmd[buffer_size];
        //read command from peer
        string cmd = "-1";
        cmd = read_from_peer(thread_info -> newsock_fd);

        cout<<cmd<<endl;
        //splitting command into vectors
        vector<string> cmd_vector;
        cmd_vector = split(string(cmd),' ');

        if(cmd_vector[0]=="create_user")
        {
            if(cmd_vector.size()==3){
                create_user(cmd_vector[1],cmd_vector[2], thread_info->newsock_fd);
                
            }
            else{
                string msg = "Few arguments, enter <user_id> <password>";
                msg = send_to_peer(thread_info->newsock_fd,msg);

            }

        }

        else if(cmd_vector[0]=="login")
        {
            if(cmd_vector.size()==3){
                login(cmd_vector[1],cmd_vector[2], thread_info->newsock_fd);
                
            }
            else{
                string msg = "Few arguments, enter <user_id> <password>";
                msg = send_to_peer(thread_info->newsock_fd,msg);
            }
        }

        else if(cmd_vector[0]=="logout")
        {
            if(cmd_vector.size()==2){
                logout(cmd_vector[1], thread_info->newsock_fd);
                
            }
            else{
                string msg = "Not a valid input format";
                msg = send_to_peer(thread_info->newsock_fd,msg);
            }
        }
        
        else if(cmd_vector[0]=="create_group")
        {

            if(cmd_vector.size()==3){
                create_group(cmd_vector[1], cmd_vector[2], thread_info->newsock_fd);
                
            }
            else{
                string msg = "Wrong input format";
                msg = send_to_peer(thread_info->newsock_fd,msg);
            }
        }

        else if(cmd_vector[0]=="join_group")
        {

            if(cmd_vector.size()==3){
                join_group(cmd_vector[1], cmd_vector[2], thread_info->newsock_fd);
                
            }
            else{
                string msg = "Wrong input format";
                msg = send_to_peer(thread_info->newsock_fd,msg);
            }
        }

        else if(cmd_vector[0]=="leave_group")
        {

            if(cmd_vector.size()==3){
                leave_group(cmd_vector[1], cmd_vector[2], thread_info->newsock_fd);
                
            }
            else{
                string msg = "Wrong input format";
                msg = send_to_peer(thread_info->newsock_fd,msg);
            }
        }

        else if(cmd_vector[0]=="requests" && cmd_vector[1]=="list_requests")
        {

            if(cmd_vector.size()==4){
                list_requests(cmd_vector[2], cmd_vector[3], thread_info->newsock_fd);
                
            }
            else{
                string msg = "Wrong input format";
                msg = send_to_peer(thread_info->newsock_fd,msg);
            }
        }

        else if(cmd_vector[0]=="accept_request")
        {
            if(cmd_vector.size()==4){
                accept_request(cmd_vector[1], cmd_vector[2],cmd_vector[3] , thread_info->newsock_fd);                
            }
            else{
                string msg = "Wrong input format";
                msg = send_to_peer(thread_info->newsock_fd,msg);
            }
        }

        else if(cmd_vector[0]=="list_groups")
        {
            if(cmd_vector.size()==1){
                list_groups(thread_info->newsock_fd);                
            }
            else{
                string msg = "Wrong input format";
                msg = send_to_peer(thread_info->newsock_fd,msg);
            }
        }

        //upload_file <file_path> <group_id> <owner_of_file>
        else if(cmd_vector[0]=="upload_file")
        {
            if(cmd_vector.size()==8){
                string file_path = cmd_vector[1];
                string group_id = cmd_vector[2];
                string owner = cmd_vector[3];
                string file_name = cmd_vector[4];
                string file_size = cmd_vector[5];
                string ip = cmd_vector[6];
                string port = cmd_vector[7];
                upload_file(file_name, group_id, owner, file_size, ip, port, file_path, thread_info->newsock_fd);
            }
            else{
                string msg = "Wrong input format";
                msg = send_to_peer(thread_info->newsock_fd,msg);
            }
        }

        //list_files <group_id>
        else if(cmd_vector[0]=="list_files")
        {
            if(cmd_vector.size()==2){
                list_files(cmd_vector[1], thread_info->newsock_fd);                
            }
            else{
                string msg = "Wrong input format";
                msg = send_to_peer(thread_info->newsock_fd,msg);
            }
        }

        //download_file​  <group_id>  <file_name>  <destination_path>
        else if(cmd_vector[0]=="download_file")
        {
            if(cmd_vector.size()==4){
                string group_id = cmd_vector[1];
                string file_name = cmd_vector[2];
                string dest_path = cmd_vector[3];

                download_file(group_id, file_name, thread_info->newsock_fd);
            }
            else{
                string msg = "Wrong input format";
                msg = send_to_peer(thread_info->newsock_fd,msg);
            }
        }
        

        else{
            string msg = "Wrong Command";
            // getline(cin,msg);
            cout<<"Acknowloedged"<<endl;
            msg = send_to_peer(thread_info->newsock_fd,msg);
        }
        // bzero(cmd,sizeof(cmd));
        cmd = "";

    }

    close(thread_info->newsock_fd);
    
    return NULL;
}


int main(int argc, char *argv[])
{
    
    FILE *fp;
	// char buf[BLK_SIZE];
	size_t sz;
	char s[20];
	int size;
	size=20;
    char trackerip1[20];
    char trackerport1[20];
    char trackerip2[20];
    char trackerport2[20];
	fp=fopen(argv[1],"r");
	if(fp==NULL)
	{
		perror("Error opening track_info.txt\n");
		exit(1);
	}
	
	if(fgets(s, size, fp) !=NULL)
	{
		
		//trackerip1=s;
		strncpy(trackerip1,s,20);
		//s='\0';
		//cout<<trackerip1<<"\n";
	}
	if(fgets(s, size, fp) !=NULL)
	{
		
		strncpy(trackerport1,s,20);
		//cout<<trackerport1<<"\n";
	}
	if(fgets(s, size, fp) !=NULL)
	{
		
		strncpy(trackerip2,s,20);
		//cout<<trackerip2<<"\n";
	}
	if(fgets(s, size, fp) !=NULL)
	{
		
		strncpy(trackerport2,s,20);
		//cout<<trackerport2<<"\n";
	}
	fclose(fp);

    int server_fd, newsockfd,port,pid;
    char buffer[buffer_size] = {0};
    struct sockaddr_in serv_addr, cli_addr;
    pthread_t thread[max_peer_count];
    info thread_info[max_peer_count];
    

    //error if arguments port not provided
    // if(argc<2){
    //     fprintf(stderr,"Error: provide port no. \n");
    //     exit(1);
    // }

    //socket (ipv4, continuos data flow, tcp)
    if((server_fd = socket(AF_INET, SOCK_STREAM, 0))<0){
        perror("Error in socket function");
        exit(1);
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
	{
		perror("setsockopt Error");
		exit(EXIT_FAILURE);
	}

    //initializing server address with 0
    bzero((char *)&serv_addr, sizeof(serv_addr));


    //port no
    // port = atoi(argv[1]);
    port = stoi(string(trackerport1));

    //updateing struct serv_add
    serv_addr.sin_family = AF_INET; //ipv4
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = inet_addr(trackerip1); //localhost

    
    //binding socket with address
    if((bind(server_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))) <0){
        perror("error in binding");
        exit(1);
    }

    
    //listen function/ waiting forconnection from client
    if(listen(server_fd,5)<0){
        perror("error in listening");
        exit(1);
    }
    int i=0;
    while(i<max_peer_count)
    {
        //accept connection
        int cli_adrr_len = sizeof(cli_addr);
        if((newsockfd = accept(server_fd,(struct sockaddr *)&cli_addr,(socklen_t *)&cli_adrr_len))<0){
            perror("Error in accepting connection");
            exit(1);
        }
        int t_port = ntohs(serv_addr.sin_port);
        thread_info[i].port = t_port;
        thread_info[i].sock_fd = server_fd;
        thread_info[i].newsock_fd = newsockfd;
        pthread_create(&thread[i] , NULL , check_input , (void*)&thread_info[i]);
        i++;

    }

    for(int i=0;i<max_peer_count;i++){
        pthread_join(thread[i],NULL);
    }

    close(newsockfd);
    close(server_fd);

    return 0;
}

// void dostuff(int sock)
// {
//     while(1)
//     {
//         char buffer[buffer_size] = {0};
//         printf("Listening....\n");
//         //readin msg from client
//         if(read(sock, buffer, sizeof(buffer))<0){
//             perror("error in reading data from client");
//             exit(1);
//         }
//         printf("Client says: %s \n",buffer);
//         //writing msg to client
//         cout<<"Enter msg to send_to_peer: ";
//         string msg = "Message received by server";
//         getline(cin,msg);
//         if(write(sock,(msg).c_str(), msg.length())<0){
//             perror("error in writing msg to client");
//             exit(1);
//         }
//     }
// }