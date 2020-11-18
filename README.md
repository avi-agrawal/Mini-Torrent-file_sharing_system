## Running format for seperate terminal:

1. Tracker file:- 
	g++ tracker.cpp -o tracker -pthread
	./tracker tracker_info.txt

2. Peer file:-
	g++ peer.cpp -o peer -pthread
	./peer <ip>:<port> tracker_info.txt
