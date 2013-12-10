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

#define PORT 9421

using namespace std;

pthread_mutex_t server_mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct thread_info {
	string userName;
	string groupName;
	int sockfd;
	struct sockaddr_in * servaddr;
	socklen_t servlen;
} thread_info;

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
		//tell other use that you are terminating connection
		//Q flag notifies that you are 'quiting' this connection
		if(send(other_users[i].sockfd, "Q", strlen("Q"),0)<0) {
			perror("ERROR sending data");
			return;
		}

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
void sendMessage(string message, string groupName, string userName, int ignoreUser) {
	#ifdef DEBUG
		cout << "Forwarding message to: " << endl;
	#endif
	for(int i=0; i<other_users.size(); i++) {
		if(i!=ignoreUser) {
			#ifdef DEBUG
				cout << "sockfd: "<<other_users[i].sockfd << " sockaddr:"<<other_users[i].ip_address<<endl;
			#endif

			//send data in the order of "T", groupName, original user, user_id, message length, and the actual message
			if(send(other_users[i].sockfd, "T", strlen("T"),0)<0) {
				perror("ERROR sending data");
				return;
			}
			stringstream ss;
			ss << message.length();

			string package = groupName+":"+userName+":"+ss.str()+":"+message+"::";
			#ifdef DEBUG
				cout << "Sending: " << package.c_str() << endl;
			#endif
			if(send(other_users[i].sockfd, package.c_str(), strlen(package.c_str()),0)<0) {
				perror("ERROR sending data");
				return;
			}
		}
	}
}

//runs on seperate thread, pings the server every two minutes to let know that still active
void *pingServer(void * input) {
	
	thread_info* info = (thread_info*)input;

	while(1) {
		usleep(100000000);//a little less than 2 minutes (to be on the safe side)
//		usleep(5000000);//a little less than 5 seconds for testing

		//send flag
		if(sendto(info->sockfd,"P", strlen("P"),0, (struct sockaddr *)info->servaddr, info->servlen) < 0) {
			perror("ERROR connecting");
			exitProgram(info->sockfd);
		}

		//send current user and group
		string result = info->groupName + ":" + info->userName + ":";
		if(sendto(info->sockfd,result.c_str(), strlen(result.c_str()),0, (struct sockaddr *)info->servaddr, info->servlen) < 0) {
			perror("ERROR connecting");
			exitProgram(info->sockfd);
		}
		#ifdef DEBUG
			cout<<"Sent Ping"<<endl;
		#endif

	}

}

//runs on seperate thread, every so often check all open TCP connections and if new messages have come in
//print them to the user but also forward to everyone else
void *checkForMessages(void * input) {

	thread_info* info = (thread_info*)input;

	//on a timer, check all connections and see if any new messages have come in
	while(1) {
		usleep(1000000);

		bool received = false;

		//check all connections
		for(int i=0; i<other_users.size(); i++) {

			socklen_t len = sizeof(other_users[i].sockaddr);

			char flag[2];
			bzero(flag, sizeof(flag));

			//if the Text flag is received, move on
			if(recv(other_users[i].sockfd, flag, 1,0)>=0) {
//				perror("Error in receiving message");
//				cout << other_users[i].ip_address << endl;
				#ifdef DEBUG
					cout << "flag: " <<flag << endl;
				#endif
				if(strcmp(flag,"Q")==0) { //flag for closing the connection
					close(other_users[i].sockfd);
					other_users.erase(other_users.begin()+i);// remove from the other_users vector
					break;
				}

				else if(strcmp(flag,"T")==0) { //flag for text message, so start reading in everything
					char package[1024];
					bzero(package, sizeof(package));
					if(recv(other_users[i].sockfd, package, sizeof(package),0)<0) {
						perror("ERROR receiving data");
					}
					#ifdef DEBUG
						cout << "package: " <<package<< endl;
					#endif

					string groupName, userName, messageLen, message;

					stringstream ss(package);
					getline(ss, groupName, ':');
					getline(ss, userName, ':');
					getline(ss, messageLen, ':');
					getline(ss, message, ':');

					if(info->groupName == groupName) {
						if(!received)
							cout << endl;

						received= true;
						//receive everything else...
						cout << " From: "<<userName<<">"<<message << endl;

						//And forward to everyone else
						sendMessage(message,groupName,userName,i);
					}
				}
			}
		}
		if(received) {
			cout << info->groupName<<">";
			cout << flush;
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

		vector<string> user_ips;
		string s;
		stringstream ss(line);
		int i=0;
		cout<<"List of Users.."<<endl;
		//parse the list
		while(getline(ss, s, ':')){ 
			if(!s.empty())//the last one's will be empty (because of the double "::") so ignore them
			if(i%2==0) { //even number so username
				cout<<s<<endl;
				if(s == userName){ //ourself
					getline(ss,s,':');
					i++;
				}
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
				//set socket to NONBlocking
				int on = fcntl(cli_sockfd,F_GETFL);
				on = (on | O_NONBLOCK);
				if(fcntl(cli_sockfd,F_SETFL,on) < 0)
				{
					perror("turning NONBLOCKING on failed\n");
				}

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

	setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,(const void *)1,sizeof(int));

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
	int status = getaddrinfo(ap_addr,port,&hints,&servinfo);
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

	pthread_t incoming_connections, incoming_messages, ping_server;
	

	while(1) { //main program while loop

		cout << cur_group << ">";
		cout << flush;
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

					//spawn thread to listen for incoming TCP connections
					int info;
					int status = pthread_create(&incoming_connections, NULL, listenForConnections, &info);

					//spawn thread to receive messages from incoming TCP connections
					thread_info t_info;
					t_info.groupName = cur_group;
					t_info.userName = userName;
					t_info.sockfd = sockfd;
					t_info.servaddr = &servaddr;
					t_info.servlen = sizeof(servaddr);
					status = pthread_create(&incoming_messages, NULL, checkForMessages, &t_info);

					//spawn thread to ping the server every two minutes
					status = pthread_create(&ping_server, NULL, pingServer, &t_info);

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
						//stop listening for messages
						pthread_cancel(incoming_connections);
						pthread_cancel(incoming_messages);
						pthread_cancel(ping_server);
						cur_group = "P2PChat";
					}
				}
			}
			else if(input=="send"){ //chat
				cout << "Send>";
				getline(cin, input);
				sendMessage(input,listName, userName,-1);
			}
		}
	}
	close(sockfd);
	return 0;
} //end main
