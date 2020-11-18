#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <bits/stdc++.h>
// #include <string.h>
using namespace std;

//global
int max_peer_count = 5;
int buffer_size = 2048;
string curr_user_login = "";
map<string, pair<string,string>> file_info;      //map<file_name,pair<file_path,file_size> file_info;

//functions
void *client(void *thread_info_ip);
string read_from_server(int socket);
string send_to_server(int socket, string msg);
vector<string> split(string str,char ch);


class info{
    public:
        string port;
        string host;
};

class info_thread_class{
    public:
        int port;
        int sock_fd;
        int newsock_fd;
};

class download_info{
    public:
        string ip;
        string port;
        string file_name;
        string dest_path;
};

info server_info;

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
string send_to_server(int socket, string msg)
{
    int check = send(socket,msg.c_str(),msg.size(),0);
    if(check < 0){
        perror("Error in sending msg to client");
    }
    msg = "";
    return msg;
}

//function to read msg from peer
string read_from_server(int socket)
{
    char buffer[buffer_size] = {0};
    bzero(buffer,sizeof(buffer));
    int check = read(socket,buffer,sizeof(buffer));
    if(check < 0){
        perror("Error in sending msg to client");
    }
    string msg = string(buffer);
    bzero(buffer,sizeof(buffer));
    return msg;
}

//download function from server
void *download(void *thread_info_ip)
{
    info_thread_class *thread_info = (info_thread_class *)thread_info_ip;
    cout<<"inside download function"<<endl;

    string msg;
    msg = read_from_server(thread_info->newsock_fd);
    cout<<msg<<endl;
    // msg = "Downloaded";
    // cout<<msg<<endl;
    // msg = send_to_server(thread_info->newsock_fd,msg);
    cout<<"Downloading..."<<endl;

    vector<string> msg_vector = split(msg,' ');
    string file_name = msg_vector[0];
    string dest_path = msg_vector[1];
    FILE *from_file, *to_file;
    char c;
    if((from_file=fopen((file_info[file_name].first).c_str(),"r"))==NULL){
        perror("Error in source file");
        return NULL;
    }
    dest_path = dest_path + "/" + file_name;
    if((to_file=fopen(dest_path.c_str(),"w+"))==NULL){
        perror("Error in destination file");
        return NULL;
    }
    
    // while(!feof(from_file)){
    //     c=fgetc(from_file);
    //     fputc(c,to_file);
    // }

    while(!feof(from_file)) {
        c = fgetc(from_file);
        if(ferror(from_file)) {
            printf("Error reading source file.\n");
            exit(1);
        }
        if(!feof(from_file)) fputc(c, to_file);
        if(ferror(to_file)) {
            printf("Error writing destination file.\n");
            exit(1);
        }
    }
    
    fclose(from_file);
    fclose(to_file);

    return NULL;
}

//server function for peer
void *server(void *thread_info_ip)
{
    cout<<"inside server of client: "<<server_info.port<<endl;
    int opt =1;
    int server_fd, newsockfd,port,pid;
    char buffer[buffer_size] = {0};
    struct sockaddr_in serv_addr, cli_addr;
    pthread_t thread[max_peer_count];
    info_thread_class thread_info[max_peer_count];
    

    //socket (ipv4, continuos data flow, tcp)
    if((server_fd = socket(AF_INET, SOCK_STREAM, 0))<0){
        perror("Error in socket function");
        exit(1);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
	{
		perror("setsockopt Error");
		exit(EXIT_FAILURE);
	}

    //initializing server address with 0
    bzero((char *)&serv_addr, sizeof(serv_addr));


    //updateing struct serv_add
    serv_addr.sin_family = AF_INET; //ipv4
    serv_addr.sin_port = htons(stoi(server_info.port));
    serv_addr.sin_addr.s_addr = inet_addr((server_info.host).c_str()); //giver ip

    
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
        cout<<"accepted....................."<<endl;
        int t_port = ntohs(serv_addr.sin_port);
        thread_info[i].port = t_port;
        thread_info[i].sock_fd = server_fd;
        thread_info[i].newsock_fd = newsockfd;
        cout<<"creating thread"<<endl;
        pthread_create(&thread[i] , NULL , download , (void*)&thread_info[i]);
        i++;

    }

    for(int i=0;i<max_peer_count;i++){
        pthread_join(thread[i],NULL);
    }
    cout<<"after join in server fucntion"<<endl;

    close(newsockfd);
    close(server_fd);

    return NULL;
}

//client_for_peer
void *connect_to_peer(void *thread_info_ip)
{
    cout<<"in client for peer function"<<endl;
    download_info *peer_info = (download_info *)thread_info_ip;
    int peer_client_fd, peer_port;
    struct sockaddr_in peer_serv_addr;
    struct hostent *peer_server;
    char buffer[buffer_size] = {0};

    //socket creation
    peer_port = stoi(peer_info->port);
    if((peer_client_fd = socket(AF_INET, SOCK_STREAM, 0))<0){
        perror("error in creating socket of this client");
        exit(1);
    }


    //updating struct serv_addr data
    bzero((char *) &peer_serv_addr, sizeof(peer_serv_addr));
    peer_serv_addr.sin_family = AF_INET;
    peer_serv_addr.sin_port = htons(peer_port);
    // bcopy((char *)server->h_addr,(char *) &serv_addr.sin_addr.s_addr,server->h_length);

    if(inet_pton(AF_INET,peer_info->ip.c_str(), &peer_serv_addr.sin_addr)<0) 
	{ 
		printf("\nInvalid address/ Address not supported \n"); 
		// return -1;
        return NULL;
	} 

    //connecting client to server
    if(connect(peer_client_fd, (struct sockaddr *)&peer_serv_addr, sizeof(peer_serv_addr))<0){
        perror("Error in connecting with server");
        exit(1);
    }

    // download();
    cout<<"connected"<<endl;
    string msg = "downloading...";
    cout<<msg<<endl;
    msg = peer_info->file_name + " " + peer_info->dest_path;
    msg = send_to_server(peer_client_fd,msg);
    
    // msg = read_from_server(peer_client_fd);
    // cout<<msg<<endl;
    cout<<"received"<<endl;

    // close(peer_client_fd);

    return NULL;
}

//peer to peer connecting function
void download_from_peer(string ip_peer, string port_peer, string file_name, string dest_path,string file_path)
{

    // FILE *from_file, *to_file;
    // char c;
    // if((from_file=fopen(file_path.c_str(),"r"))==NULL){
    //     perror("Error in source file");
    //     return;
    // }
    // dest_path = dest_path + "/" + file_name;
    // if((to_file=fopen(dest_path.c_str(),"w+"))==NULL){
    //     perror("Error in destination file");
    //     return;
    // }
    
    // // while(!feof(from_file)){
    // //     c=fgetc(from_file);
    // //     fputc(c,to_file);
    // // }

    // while(!feof(from_file)) {
    //     c = fgetc(from_file);
    //     if(ferror(from_file)) {
    //         printf("Error reading source file.\n");
    //         exit(1);
    //     }
    //     if(!feof(from_file)) fputc(c, to_file);
    //     if(ferror(to_file)) {
    //         printf("Error writing destination file.\n");
    //         exit(1);
    //     }
    // }
    
    // fclose(from_file);
    // fclose(to_file);

    pthread_t client_thread_peer;
    download_info peer_info;
    peer_info.port = string(port_peer);
    peer_info.ip = string(ip_peer);
    peer_info.file_name = file_name;
    peer_info.dest_path = dest_path;
    cout<<"in download_from_peer function"<<endl;
    pthread_create(&client_thread_peer, NULL , connect_to_peer , (void *)&peer_info);
    pthread_join(client_thread_peer,NULL);
    // pthread_exit(NULL);
    cout<<"out of download_from_peer function"<<endl;

    
    // return;
}

//client function to take commands
void *client(void *thread_info_ip)
{
    
    info *tracker_info = (info *)thread_info_ip;
    int client_fd, port;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    char buffer[buffer_size] = {0};

    //socket creation
    port = stoi(tracker_info->port);
    if((client_fd = socket(AF_INET, SOCK_STREAM, 0))<0){
        perror("error in creating socket of this client");
        exit(1);
    }

    //setting up host
    // server = gethostbyname((tracker_info->host).c_str());
    // if(server ==NULL){
    //     perror("Error, no such host exist");
    //     exit(1);
    // }

    //updating struct serv_addr data
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    // bcopy((char *)server->h_addr,(char *) &serv_addr.sin_addr.s_addr,server->h_length);

    if(inet_pton(AF_INET, tracker_info->host.c_str(), &serv_addr.sin_addr)<0) 
	{ 
		printf("\nInvalid address/ Address not supported \n"); 
		// return -1;
        return NULL;
	} 

    //connecting client to server
    if(connect(client_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))<0){
        perror("Error in connecting with server");
        exit(1);
    }

    while(1)
    {
        //reading and writing msg/data
        printf("\nEnter command: ");
        string cmd;
        getline(cin,cmd);
        if(cmd == ""){
            cout<<"Enter any command"<<endl;
            cmd = "";
            continue;
        }

        vector<string> cmd_vector = split(cmd,' ');

        if(cmd_vector[0]=="create_user")
        {
            cmd = send_to_server(client_fd, cmd);
            string msg = read_from_server(client_fd);
            printf("Server says: %s \n",msg.c_str());
        }

        else if(cmd_vector[0]=="login")
        {
            //if peer already logged into any user
            if(curr_user_login != ""){
                cout<<"You are already logged into: "<<curr_user_login<<endl;
            }
            //if curr_user_login is empty
            else{
                cmd = send_to_server(client_fd, cmd);
                string msg = read_from_server(client_fd);
                printf("Server says: %s \n",msg.c_str());
                if(msg == "Logged into the user"){
                    curr_user_login = cmd_vector[1];
                }
            }
        }

        else if(cmd_vector[0]=="logout")
        {
            cmd = cmd + " " + curr_user_login;
            cmd = send_to_server(client_fd, cmd);
            string msg = read_from_server(client_fd);
            printf("Server says: %s \n",msg.c_str());
            curr_user_login = "";
        }

        else if(cmd_vector[0]=="create_group")
        {
            if(curr_user_login == ""){
                cout<<"First login into any user"<<endl;
                // continue;
            }
            else{
                cout<<"user logged in: "<<curr_user_login<<endl;
                string mod_cmd = cmd + " " + curr_user_login;
                cout<<mod_cmd<<endl;
                cmd = send_to_server(client_fd, mod_cmd);
                mod_cmd = "";
                string msg = read_from_server(client_fd);
                printf("Server says: %s \n",msg.c_str());
            }
        }

        else if(cmd_vector[0]=="join_group")
        {
            if(curr_user_login == ""){
                cout<<"First login into any user"<<endl;
                // continue;
            }
            else{
                cout<<"user logged in: "<<curr_user_login<<endl;
                string mod_cmd = cmd + " " + curr_user_login;
                cout<<mod_cmd<<endl;
                cmd = send_to_server(client_fd, mod_cmd);
                mod_cmd = "";
                string msg = read_from_server(client_fd);
                printf("Server says: %s \n",msg.c_str());
            }
        }

        else if(cmd_vector[0]=="leave_group")
        {
            if(curr_user_login == ""){
                cout<<"First login into any user"<<endl;
                // continue;
            }
            else{
                cout<<"user logged in: "<<curr_user_login<<endl;
                string mod_cmd = cmd + " " + curr_user_login;
                cout<<mod_cmd<<endl;
                cmd = send_to_server(client_fd, mod_cmd);
                mod_cmd = "";
                string msg = read_from_server(client_fd);
                printf("Server says: %s \n",msg.c_str());
            }
        }

        else if(cmd_vector[0]=="requests" && cmd_vector[1]=="list_requests")
        {
            if(curr_user_login == ""){
                cout<<"First login into any user"<<endl;
                // continue;
            }
            else{
                cout<<"user logged in: "<<curr_user_login<<endl;
                string mod_cmd = cmd + " " + curr_user_login;
                cout<<mod_cmd<<endl;
                cmd = send_to_server(client_fd, mod_cmd);
                mod_cmd = "";
                string msg = read_from_server(client_fd);
                printf("Server says: %s \n",msg.c_str());
            }
        }

        else if(cmd_vector[0]=="accept_request")
        {
            if(curr_user_login == ""){
                cout<<"First login into any user"<<endl;
                // continue;
            }
            else{
                cout<<"user logged in: "<<curr_user_login<<endl;
                string mod_cmd = cmd + " " + curr_user_login;
                cout<<mod_cmd<<endl;
                cmd = send_to_server(client_fd, mod_cmd);
                mod_cmd = "";
                string msg = read_from_server(client_fd);
                printf("Server says: %s \n",msg.c_str());
            }
        }

        else if(cmd_vector[0]=="list_groups")
        {
            cmd = send_to_server(client_fd, cmd);
            string msg = read_from_server(client_fd);
            printf("Server says: %s \n",msg.c_str());
        }

        else if(cmd_vector[0]=="upload_file")
        {
            if(curr_user_login == ""){
                cout<<"First login into any user"<<endl;
                // continue;
            }
            else{
                cout<<"user logged in: "<<curr_user_login<<endl;

                //getting file_size
                int file_size;
                string file_path;
                {
                    file_path = cmd_vector[1];
                    ifstream in(file_path,ios::ate|ios::binary);
                    if(in.fail()){
                        std::cout << "wrong file path or name" << endl;
                        continue;
                    }
                    file_size = in.tellg();
                    in.close();
                }

                vector<string> file_path_vector = split(cmd_vector[1],'/');
                string file_name = file_path_vector[file_path_vector.size()-1];
                string mod_cmd = cmd + " " + curr_user_login + " " + file_name + " " + to_string(file_size);
                mod_cmd = mod_cmd + " " + server_info.host + " " + server_info.port;
                cout<<mod_cmd<<endl;
                cmd = send_to_server(client_fd, mod_cmd);
                mod_cmd = "";
                string msg = read_from_server(client_fd);
                printf("Server says: %s \n",msg.c_str());
                if(msg == "1" || msg == "2"){
                    file_info[file_name];
                    file_info[file_name] = make_pair(file_path,file_size);
                    cout<<"File uploaded\n"<<endl;
                }
                else{
                    cout<<msg<<endl;
                }
                
            }
        }

        else if(cmd_vector[0]=="list_files")
        {
            cmd = send_to_server(client_fd, cmd);
            string msg = read_from_server(client_fd);
            printf("Server says: %s \n",msg.c_str());
        }


        //download_file​  <group_id>  <file_name>  <destination_path>
        else if(cmd_vector[0]=="download_file")
        {
            if(curr_user_login == ""){
                cout<<"First login into any user"<<endl;
                // continue;
            }
            else{
                string dest_path = cmd_vector[3];
                string file_name = cmd_vector[2];
                cout<<"user logged in: "<<curr_user_login<<endl;
                cmd = send_to_server(client_fd, cmd);
                string msg = read_from_server(client_fd);

                //msg = res:ip:port of the client having the file
                vector<string> ip_port_vector = split(msg,':');
                if(ip_port_vector[0] == "1"){
                    printf("Server says: %s \n",msg.c_str());
                    string file_path = ip_port_vector[3];

                    //connect to peer's server
                    download_from_peer(ip_port_vector[1],ip_port_vector[2],file_name,dest_path,file_path);
                }
                else{
                    printf("Server says: %s \n",msg.c_str());
                }
            }
        }

        
        else
        {
            // strcpy(buffer, msg.c_str());
            cmd = send_to_server(client_fd, cmd);
            string msg = read_from_server(client_fd);   
            printf("Server says: %s \n",msg.c_str());
        }


        
        cmd = "";
    }

    close(client_fd);

    return NULL;
}

int main(int argc, char *argv[])
{   
    int opt = 1;
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
	fp=fopen(argv[2],"r");
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

    //error if hostname and port not given
    if(argc <3){
        fprintf(stderr, "enter hostname,port_no and tracker_info file");
        exit(1);
    }

    
    pthread_t client_thread,server_thread;

    info tracker_info;
    tracker_info.port = string(trackerport1);
    tracker_info.host = string(trackerip1);

    vector<string> ip_port_vector = split(argv[1],':');
    server_info.host = ip_port_vector[0];
    server_info.port = ip_port_vector[1];

    pthread_create(&client_thread, NULL , client , (void *)&tracker_info);
    pthread_create(&server_thread, NULL , server , (void *)&server_info);

    pthread_join(server_thread,NULL);
    pthread_join(client_thread,NULL);
    // pthread_exit(NULL);

    
    return 0;
}