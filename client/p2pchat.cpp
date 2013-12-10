/* Jonathan Cobian, Chas Jhin, Jason Wassel 
 * CSE 30264 - Computer Networks 
 * Project 3- P2P Chat
 */

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string>
#include <iostream>
#include <sstream>
#include <strings.h>
#include <cstring>
#include <errno.h>
#include <sys/time.h>
#include <time.h>
#include <vector>

#include <pthread.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

//#define DEBUG

#define PORT 9425

//TODO when one person leaves all of a sudden everyone else starts getting newlines
//in their chat window... why?....

//TODO ADVANCED:
//ping the server every 2 minutes to 'stay alive'
//reconnection behavior if a user leaves for the remaining users
//file transfer
//nicer UI

using namespace std;

pthread_mutex_t server_mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct user {
	int sockfd;
	struct sockaddr_in sockaddr;
	string ip_address;
} user;

vector<user> other_users; //vector of ips representing all users currently connected to

//close all connections to other users
//effectively do so by iterating through other_users and closing the connections
void closeAllConnections()
{
	for(int i=0; i<other_users.size(); i++) {

		close(other_users[i].sockfd);

		#ifdef DEBUG
			cout << "closing connection to: "<<other_users[i].ip_address<<endl;
		#endif
	}

	//finally clear the other_users vector
	other_users.clear();	
}

//because we have a fair amount of connections open we want to close everything gracefully before quiting the program (if possible)
//sockfd should be the connection to the server
void exitProgram(int sockfd) 
{
	closeAllConnections();

	close(sockfd); 
	exit(1);
}

//send a message to all the other users in other_users
void sendMessage(string message) {
	#ifdef DEBUG
		cout << "Forwarding message to: ";
	#endif
	for(int i=0; i<other_users.size(); i++) {
		#ifdef DEBUG
			cout << "sockfd: "<<other_users[i].sockfd << " sockaddr:"<<other_users[i].ip_address<<endl;
		#endif
		
		if(send(other_users[i].sockfd, message.c_str(), message.length(),0)<0) {
			perror("ERROR sending data");
		}
	}
}

//runs on seperate thread, every so often check all open TCP connections and if new messages have come in
//print them to the user but also forward to everyone else
void *checkForMessages(void * input) {

	//on a timer, check all connections and see if any new messages have come in
	while(1) {
		usleep(1000000);

		//check all connections
		for(int i=0; i<other_users.size(); i++) {

			socklen_t len = sizeof(other_users[i].sockaddr);

			char message[1024];
			bzero(message, sizeof(message));

			//if a message was received print it out
			if(recv(other_users[i].sockfd, message, sizeof(message),0)>=0){
				cout << message << endl;

				//And forward to everyone else
				for(int j=0; j<other_users.size(); j++) {
					if(i!=j) { //don't send message back to sender
						if(send(other_users[j].sockfd, message, sizeof(message),0)<0) {
							perror("ERROR sending data");
						}					
					}
				}
			}
		}
	}
}

//contact the server and attempt to join list listName with nickname userName
bool joinList(int sockfd, struct sockaddr_in * servaddr, socklen_t servlen, string listName, string userName)
{
	//send flag
	if(sendto(sockfd,"J", strlen("J"),0, (struct sockaddr *) servaddr, servlen) < 0) {
		perror("ERROR connecting");
		exitProgram(sockfd);
	}
	string result = listName + ":" + userName + ":";
	if(sendto(sockfd,result.c_str(),strlen(result.c_str()),0, (struct sockaddr *) servaddr, servlen) < 0) {
		perror("ERROR connecting");
		exitProgram(sockfd);
	}
	#ifdef DEBUG
		cout<<"Sent list and user of "<<result<<endl;
	#endif

	//receive message type
	char recvline[1024];
	bzero(recvline, sizeof(recvline));

	if(recvfrom(sockfd,recvline,sizeof(recvline),0,(struct sockaddr *)servaddr,&servlen) < 0){ //this should be one byte char
		perror("ERROR connecting");
		exitProgram(sockfd);
	}

	string mesgType;
	//bzero(&mesgType, sizeof(mesgType));
	mesgType = recvline;
	#ifdef DEBUG
		cout<<"Received message type of "<<mesgType<<endl;
	#endif	
	if(mesgType == "S") //success
	{
		bzero(recvline, sizeof(recvline));	
		if(recvfrom(sockfd,recvline,sizeof(recvline),0,(struct sockaddr *)servaddr,&servlen) < 0){ //this should be one byte char
			perror("ERROR connecting");
			exitProgram(sockfd);
		}
		string line = recvline;
		#ifdef DEBUG
//			cout<<"Received groupUser of "<<line<<endl;
		#endif

		if(line == "::"){
			cout <<"Group did not exist so new group was created"<<endl;
		}
		else {
			vector<string> user_ips;
			string s;
			stringstream ss(line);
			int i=0;
			cout<<"List of Users.."<<endl;
			while(getline(ss, s, ':')){ 
				if(!s.empty())//the last one's will be empty (because of the double "::") so ignore them
				if(i%2==0) { //even number so username
					cout<<s<<endl;
				}else { //odd number so ip address
					#ifdef DEBUG
						cout<<s<<endl;
					#endif
					user_ips.push_back(s);
				}	
				i++;
			}

			//now try to connect to one of the other ips
			for(int i=0; i<user_ips.size(); i++)
			{
				//try to connect to the ip on port 9421
				
				int cli_sockfd, n;
				struct sockaddr_in cliaddr;

				//create the socket
				cli_sockfd=socket(AF_INET,SOCK_STREAM,0);
				if(cli_sockfd < 0) {
						perror("ERROR opening socket");
				}

				//build server inet address
				bzero(&cliaddr,sizeof(cliaddr));
				cliaddr.sin_family = AF_INET;
				cliaddr.sin_addr.s_addr=inet_addr((user_ips[i].c_str())); //the address
				cliaddr.sin_port=htons(PORT); //the port

				//connect
				if(connect(cli_sockfd,(struct sockaddr *) &cliaddr, sizeof(cliaddr)) < 0) {
					#ifdef DEBUG
						cout <<"ERROR connecting to: cli_sockfd="<<cli_sockfd<<" server_ip="<<user_ips[i]<<endl;
					#endif
				}
				//connection was succesful, so add this ip to the list of other users
				else {
					#ifdef DEBUG
						cout <<"connecting to: cli_sockfd="<<cli_sockfd<<" server_ip="<<user_ips[i]<<endl;
					#endif

					//add this user to other_users
					user temp_user;
					temp_user.sockfd = cli_sockfd;
					temp_user.sockaddr = cliaddr;
					temp_user.ip_address = user_ips[i];
					other_users.push_back(temp_user);
					break;
				}
			}

			return true;
		}
	}
	else {
		//failure do something
		cout<<"Failed to join list due to server issue"<<endl;
		return false;
	}
}


//notify server that we want to leave a specific list, and we are user @userName
bool leaveList(int sockfd, struct sockaddr_in * servaddr, socklen_t servlen, string listName, string userName)
{
	//send command "Q" for quit list!
	if(sendto(sockfd,"Q", strlen("Q"),0, (struct sockaddr *) servaddr, servlen) < 0) {
		perror("ERROR connecting");
		exitProgram(sockfd);
	}
	listName += ":";
	string result = listName + userName;
	result+=":";
	if(sendto(sockfd,result.c_str(),strlen(result.c_str()),0, (struct sockaddr *) servaddr, servlen) < 0) {
		perror("ERROR connecting");
		exitProgram(sockfd);
	}
	#ifdef DEBUG
		cout<<"Notified Server that this user is leaving: "<<result<<endl;
	#endif

	//receive message type
	char recvline[2];
	bzero(recvline, sizeof(recvline));

	if(recvfrom(sockfd,recvline,sizeof(recvline),0,(struct sockaddr *)servaddr,&servlen) < 0){ //this should be one byte char
		perror("ERROR connecting");
		exitProgram(sockfd);
	}

	string mesgType;
	mesgType = recvline;
	if(mesgType == "S") {//success
		return true;
	}
	else { //failure do something
		cout << "Error: Server didn't receive message to leave.  Try again or contact your network administrator"<<endl;
		return false;
	}
}

void requestList(int sockfd, struct sockaddr_in * servaddr, int servlen)
{
	//send command
	if(sendto(sockfd,"L", 2,0, (struct sockaddr *) servaddr, servlen) < 0) {
		perror("ERROR connecting");
		exitProgram(sockfd);
	}

	//receive message type
	char recvline[1024];
	bzero(recvline, sizeof(recvline));
	if(recvfrom(sockfd,recvline,2,0,NULL,NULL) < 0){ //this should be one byte char
		perror("ERROR connecting");
		exitProgram(sockfd);
	}

#ifdef DEBUG
//	printf("%s\n",recvline);
#endif

	if(strcmp(recvline, "G")==0){ 
		bzero(&recvline,sizeof(recvline));
	
		//receive actual message	
		if(recvfrom(sockfd,recvline, 1024,0,NULL,NULL) < 0) {
			perror("ERROR connecting");
			exitProgram(sockfd);
		}

		string line = recvline;

		if(line == "::") {
			printf("No currently available groups\n");	
		}
		else { //tokenize the groups by ":" and print them all out
			string s;
			stringstream ss(line);
			while(getline(ss, s, ':')){ 
				if(!s.empty())//the last one's will be empty (because of the double "::") so ignore them
					cout << s << endl;
			}
		}
	}
}



//Listen for incoming TCP connections, and if for your current group
//add the incoming user to @other_users
void *listenForConnections(void *input) {
	
	int sockfd,connfd,n;
    struct sockaddr_in servaddr, cliaddr;
    socklen_t clilen;

    //create the socket
	sockfd=socket(AF_INET,SOCK_STREAM,0);
    if(sockfd < 0) {
        perror("ERROR opening socket");
    }

    //build server inet address
	bzero(&servaddr,sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr=htonl(INADDR_ANY);

    servaddr.sin_port=htons(PORT);

    bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
    listen(sockfd,1024);

    while(1) {
        clilen = sizeof(cliaddr);
        connfd = accept(sockfd,(struct sockaddr *)&cliaddr,&clilen);

		#ifdef DEBUG
			printf("New client accepted!\n");
			printf("\tNew client address:%s\n",inet_ntoa(cliaddr.sin_addr));
			printf("\tClient port:%d\n",ntohs(cliaddr.sin_port));
		#endif

		//set socket to NONBlocking
		int on = fcntl(connfd,F_GETFL);
		on = (on | O_NONBLOCK);
		if(fcntl(connfd,F_SETFL,on) < 0)
		{
			perror("turning NONBLOCKING on failed\n");
		}

		//if were're in the correct group, add the info to other_users
		user temp_user;
		temp_user.sockfd = connfd;
		temp_user.sockaddr = cliaddr;
		temp_user.ip_address = inet_ntoa(cliaddr.sin_addr);
		other_users.push_back(temp_user);
	}

	return 0;
}

int main(int argc, char **argv)
{
	if(argc!=2)
	{
		printf("usage: ./p2pchat <IP address>\n");
		exit(1);
	}

	//spawn thread to listen for incoming TCP connections
	int info;
	pthread_t thread;
	int status = pthread_create(&thread, NULL, listenForConnections, &info);

	//spawn thread to receive messages from incoming TCP connections
	int info2;
	pthread_t thread2;
	status = pthread_create(&thread2, NULL, checkForMessages, &info2);

	#ifdef DEBUG
		if(status)
			printf("Error creating thread for TCP listening: %i\n", status);
		else
			printf("Listening for TCP connections (Thread created)\n");
	#endif

	int sockfd, n;
	struct sockaddr_in servaddr;

	//set up ports and ip based on user input
	char* pch = argv[1];
	char* ap_addr = strtok (pch,":");
	char* port = strtok(NULL, "");

	//convert hostname into ip addr
	struct addrinfo hints;
	struct addrinfo *servinfo;
	memset(&hints,0,sizeof(hints)); //weird, you need the memset or else it won't work
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	//get addr info of the ip address, feed it the hints, results stored
	//in the servinfo which is a linked list
	status = getaddrinfo(ap_addr,port,&hints,&servinfo);
	if(status!=0) {
		printf("getaddrinfo: %s\n",gai_strerror(status));
		exit(1);
	}
	char ipstr[INET_ADDRSTRLEN];
	struct addrinfo *p;
	for(p=servinfo;p!=NULL;p=p->ai_next) {
		void *addr;
		//char *ipver;
		if(p->ai_family = AF_INET) {
			struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
			addr = &(ipv4->sin_addr);
			//ipver = "IPv4";
		}
		//convert IP to a string, store in ipstr
		inet_ntop(p->ai_family,addr,ipstr,sizeof ipstr);
	}
	//this was causing me compilation problems, hopefully will fix it later? -CHAS
	//free(addrinfo);	

	//create the socket
    sockfd=socket(AF_INET,SOCK_DGRAM,0);
    if(sockfd < 0) {
		perror("ERROR opening socket");
		exit(1);
    }

	//build server inet address
    bzero(&servaddr,sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr=inet_addr(ipstr); //the address
    servaddr.sin_port=htons(atoi(port)); //the port

	printf("Welcome to the P2P Client\n");
	printf("Connecting to %s:%s...\n", ap_addr, port);

	string input;

	string cur_group = "P2PChat";
	string listName = "none";
	string userName = "none";

	while(1) { //main program while loop

		cout << cur_group << ">";
		getline(cin, input);

		if(cur_group == "P2PChat"){ //Not currently in a chat group
			if(input== "list"){
				requestList(sockfd, &servaddr, sizeof(servaddr));
			}
			else if(input.substr(0, input.find(' ')) == "join"){
				//- join the group "groupname" as the user "username". 
				//Restrict both of these to single word names with printable characters.
				stringstream ss(input);
				string temp;
				ss >> temp;
				ss >> listName;
				ss >> userName;
				if(joinList(sockfd, &servaddr, sizeof(servaddr), listName, userName)){
					//if joinList was succesful, change the group name
					cur_group = listName;
				}
			}
			else if(input == "quit"){
				printf("Bye!\n");
				//- exit the program, closing all group connections.
				exitProgram(sockfd);
			}
			else {
				printf("Not a valid command\n");
			}
		}
		else { //Currently in a chat group!
	
			//both leave and quit should tell server and close all connections gracefully
			if(input == "leave" || input == "quit"){
				if(leaveList(sockfd, &servaddr, sizeof(servaddr), listName, userName)){
					closeAllConnections();	

					if(input=="quit") { //assuming everything closed correctly, now quit 
						printf("Bye!\n");
						exitProgram(sockfd);
					}
					else { //'leave'
						//TODO the user could optionally join 
						//another group.
						cur_group = "P2PChat";
					}
				}
			}
			else { //chat
				sendMessage(input);
			}
		}
	}
	close(sockfd);
	return 0;
} //end main
