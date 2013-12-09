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
#define DEBUG

#define PORT 9425

//TODO actually send the message to everyone
//If you receive a message, pass it on to everyone else
//when you leave, tell the server
//
//ADVANCED:
//ping the server every 2 minutes to 'stay alive'
//reconnection behavior if a user leaves
//file transfer
//nicer UI

using namespace std;

pthread_mutex_t server_mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
    int connfd;
    struct sockaddr_in cliaddr;
    socklen_t clilen;

} thread_info;

typedef struct user {
	int sockfd;
	struct sockaddr_in sockaddr;
	string ip_address;
} user;

vector<user> other_users; //vector of ips representing all users currently connected to

//send a message to all the other users in other_users
void sendMessage(string message) {
	#ifdef DEBUG
		cout << "Forwarding message to: ";
	#endif
	for(int i=0; i<other_users.size(); i++) {
		cout << "sockfd: "<<other_users[i].sockfd << " sockaddr:"<<other_users[i].ip_address<<endl;
		//actually forward the message here
		
		//sendto(sockfd, message.c_str(), message.length(),0,(struct sockaddr *) &servaddr, sizeof(servaddr));
	}
}

bool joinList(int sockfd, struct sockaddr_in * servaddr, socklen_t servlen, string listName, string userName)
{
	//send command
	if(sendto(sockfd,"J", strlen("J"),0, (struct sockaddr *) servaddr, servlen) < 0) {
		perror("ERROR connecting");
		exit(1);
	}
	#ifdef DEBUG
//	cout<<"Sent message type of J"<<endl;
	#endif
	listName += ":";
	string result = listName + userName;
	result+=":";
	if(sendto(sockfd,result.c_str(),strlen(result.c_str()),0, (struct sockaddr *) servaddr, servlen) < 0) {
		perror("ERROR connecting");
		exit(1);
	}
	#ifdef DEBUG
		cout<<"Sent list and user of "<<result<<endl;
	#endif

	//receive message type
	char recvline[1024];
	bzero(recvline, sizeof(recvline));

	if(recvfrom(sockfd,recvline,sizeof(recvline),0,(struct sockaddr *)servaddr,&servlen) < 0){ //this should be one byte char
		perror("ERROR connecting");
		exit(1);
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
			exit(1);
		}
		string line = recvline;
		#ifdef DEBUG
			cout<<"Received groupUser of "<<line<<endl;
		#endif

		vector<string> user_ips;
		string s;
		stringstream ss(line);
		int i=0;
		cout<<"List of Users.."<<endl;
		while(getline(ss, s, ':')){ 
			if(!s.empty())//the last one's will be empty (because of the double "::") so ignore them
			if(i%2==0) { //even number so username
				cout<<s<<":";
			}else { //odd number so ip address
				cout<<s<<endl;
				user_ips.push_back(s);
			}	
			i++;
		}

		//now try to connect to one of the other ips
		for(int i=0; i<user_ips.size(); i++)
		{
			cout << "Attempting to connect to: "<< user_ips[i]<<endl;

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
			if(connect(cli_sockfd,(struct sockaddr *) &cliaddr, sizeof(cliaddr)) < 0) 
					perror("ERROR connecting");
			//connection was succesful, so add this ip to the list of other users
			else {
				cout <<"connecting to: cli_sockfd="<<cli_sockfd<<" server_ip="<<user_ips[i]<<endl;
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
	else {
		//failure do something
		cout<<"Failed to join list due to server issue"<<endl;
		return false;
	}
}


//notify server that we want to leave a specific list, and we are user @userName
bool leaveList(int sockfd, struct sockaddr_in * servaddr, socklen_t servlen, string listName, string userName)
{
	//send command
	if(sendto(sockfd,"L", strlen("L"),0, (struct sockaddr *) servaddr, servlen) < 0) {
		perror("ERROR connecting");
		exit(1);
	}
	listName += ":";
	string result = listName + userName;
	result+=":";
	if(sendto(sockfd,result.c_str(),strlen(result.c_str()),0, (struct sockaddr *) servaddr, servlen) < 0) {
		perror("ERROR connecting");
		exit(1);
	}
	#ifdef DEBUG
		cout<<"Notified Server that this user is leaving: "<<result<<endl;
	#endif

	//receive message type
	char recvline[2];
	bzero(recvline, sizeof(recvline));

	if(recvfrom(sockfd,recvline,sizeof(recvline),0,(struct sockaddr *)servaddr,&servlen) < 0){ //this should be one byte char
		perror("ERROR connecting");
		exit(1);
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
		exit(1);
	}

	//receive message type
	char recvline[1024];
	bzero(recvline, sizeof(recvline));
	if(recvfrom(sockfd,recvline,2,0,NULL,NULL) < 0){ //this should be one byte char
		perror("ERROR connecting");
		exit(1);
	}

#ifdef DEBUG
//	printf("%s\n",recvline);
#endif

	if(strcmp(recvline, "G")==0){ 
		bzero(&recvline,sizeof(recvline));
	
		//receive actual message	
		if(recvfrom(sockfd,recvline, 1024,0,NULL,NULL) < 0) {
			perror("ERROR connecting");
			exit(1);
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

//close all connections to other users
//effectively do so by iterating through other_users and closing the connections
void closeAllConnections()
{
	for(int i=0; i<other_users.size(); i++) {

		//close(other_users[i]);
		//cout << "closing connection to: "<<other_users[i]<<endl;

	}

	//finally clear the other_users vector
	other_users.clear();	
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

        printf("New client accepted!\n");
        printf("\tNew client address:%s\n",inet_ntoa(cliaddr.sin_addr));
        printf("\tClient port:%d\n",cliaddr.sin_port);

		//TODO if were're in the correct group, add the info to
		//other_users
		user temp_user;
		temp_user.sockfd = sockfd;
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
	//CHAS: this was causing me compilation problems, hopefully will fix it later?
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
			if(input== "list") {
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
				exit(1);
			}
			else {
				printf("Not a valid command\n");
			}
		}
		else { //Currently in a chat group!

			if(input == "leave" || input == "quit"){ //both leave and quit should tell server and close all connections gracefully
				if(leaveList(sockfd, &servaddr, sizeof(servaddr), listName, userName)){
					//if leaveList was succesful, close all other connections
					closeAllConnections();	

					if(input=="quit") { //assuming everything closed correctly, now quit 
						printf("Bye!\n");
						exit(1); //- exit the program 
					}
					else { //'leave'
						//TODO the user could optionally join 
						//another group.
						cur_group = "P2PChat";
					}
				}
			}
			else {
				//TODO chat
				cout << "Sending: " << input << endl;
				sendMessage(input);
			}
		}
	}

	
	close(sockfd);
	return 0;
} //end main
