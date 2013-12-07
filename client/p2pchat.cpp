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

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#define DEBUG

using namespace std;


void joinList(int sockfd, struct sockaddr_in * servaddr, socklen_t servlen, string listName, string userName)
{
	//send command
	if(sendto(sockfd,"J", strlen("J"),0, (struct sockaddr *) servaddr, servlen) < 0) {
		perror("ERROR connecting");
		exit(1);
	}
	#ifdef DEBUG
	cout<<"Sent message type of J"<<endl;
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
/*
	userName += ":";
	if(sendto(sockfd,userName.c_str(),strlen(userName.c_str()),0, (struct sockaddr *) servaddr, servlen) < 0) {
		perror("ERROR connecting");
		exit(1);
	}
*/
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
				cout<<"Received groupUserCombo of "<<line<<endl;
			#endif
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
				}	
				i++;
			}
		
		
	}
	else {
		//failure do something
		cout<<"Failure!!"<<endl;
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
	printf("%s\n",recvline);
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
    	//weird, you need the memset or else it won't work
	memset(&hints,0,sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	//get addr info o fht eip address, feed it the hints, results stored
    	//in the servinfo which is  linked list
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

	while(1) {

		cout << cur_group << ">";
		getline(cin, input);

		if(cur_group == "P2PChat"){ //ie not currently in a chat group
			if(input== "list") {
				requestList(sockfd, &servaddr, sizeof(servaddr));
			}
			else if(input.substr(0, input.find(' ')) == "join"){
				//- join the group "groupname" as the user "username". 
				//Restrict both of these to single word names with printable characters.
				stringstream ss(input);
				string temp, listName, userName;
				ss >> temp;
				ss >> listName;
				ss >> userName;
				joinList(sockfd, &servaddr, sizeof(servaddr), listName, userName);
				

			}
		}
		else if(input == "leave"){
			//leave - leave the group, closing all group connections. the user could optionally join 
			//another group.
			cur_group = "P2PChat";
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

	
	close(sockfd);
	return 0;
} //end main
