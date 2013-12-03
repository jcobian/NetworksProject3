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
//#define DEBUG

using namespace std;


void joinList(int sockfd, struct sockaddr_in * servaddr, int servlen, string listName, string userName)
{
	//send command
	if(sendto(sockfd,"J", 2,0, (struct sockaddr *) servaddr, servlen) < 0) {
		perror("ERROR connecting");
		exit(1);
	}

	listName += ":";
	if(sendto(sockfd,listName.c_str(),sizeof(listName.c_str()),0, (struct sockaddr *) servaddr, servlen) < 0) {
		perror("ERROR connecting");
		exit(1);
	}

	userName += ":";
	if(sendto(sockfd,userName.c_str(),sizeof(userName.c_str()),0, (struct sockaddr *) servaddr, servlen) < 0) {
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

	//create the socket
    sockfd=socket(AF_INET,SOCK_DGRAM,0);
    if(sockfd < 0) {
		perror("ERROR opening socket");
		exit(1);
    }

	//build server inet address
    bzero(&servaddr,sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr=inet_addr(ap_addr); //the address
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
