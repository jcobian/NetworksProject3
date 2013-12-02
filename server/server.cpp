/*
 * Jason Wassel, Jonathan Cobian, Charles Jhin
 * 10//13
 * CSE 30264
 * Project 2
 *
 * Server
 */

#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <cstdlib>
#include <strings.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <pthread.h>
#include <stdlib.h>
#include <sstream>
#include "../chatinfo.h"
#define DEBUG

using namespace std;

int main(int argc, char**argv)
{

	if(argc !=2 ) {
		printf("usage: ./server <port number>\n");
		exit(1);
	}
	int portNumber = atoi(argv[1]);
	printf("Connecting on port %d\n",portNumber);
	////////////////////////////////////
	// Setup and Connection 
	// 
	int sockfd;
	struct sockaddr_in servaddr,clientaddr;

	//create the socket
	sockfd=socket(AF_INET,SOCK_DGRAM,0);
	if(sockfd < 0) {
		perror("ERROR opening socket");
		exit(1);
	}
	//build server inet address
	bzero(&servaddr,sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
	servaddr.sin_port=htons(portNumber);

	int val = 1;
	if(	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val))==-1) {
		perror("setsockopt failed");
		exit(1);
	}

	//bind to port
	if(	bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr))<0) {
		perror("bind failed");
		exit(1);
	}
	socklen_t len = sizeof clientaddr;
	char messageType;
	int ibytes,obytes;
	vector<group> activeGroups;	
	while(1) {
		ibytes = 0;
		obytes = 0;
		memset((char*)&messageType,0,sizeof(messageType));
		#ifdef DEBUG
			printf("About to receive message type");
		#endif
		if((ibytes=recvfrom(sockfd,&messageType,sizeof(messageType),0,(struct sockaddr *)&clientaddr,&len))==-1) {
			perror("Client-recvfrom() error");
			exit(1);
		}
		#ifdef DEBUG
			printf("Received: %c\n",messageType);	
		#endif
		//list
		if(messageType == 'L') {
				char sendBack = 'G';
				#ifdef DEBUG
					printf("About to send message type");
				#endif
				if((obytes=sendto(sockfd,&sendBack,1,0,(struct sockaddr *)&clientaddr,sizeof(clientaddr)))<0) {
					perror("client-sendto error");
					exit(1);
				}
				#ifdef DEBUG
					printf("Sent: %c\n",sendBack);	
				#endif
				char *groupNames;
				size_t i;
				for(i=0;i<activeGroups.size();i++) {
					strcat(groupNames,activeGroups[i].groupName);
					strcat(groupNames,":");
				}
				strcat(groupNames,":");
				#ifdef DEBUG
					printf("About to send group names");
				#endif
				if((obytes=sendto(sockfd,groupNames,sizeof(groupNames),0,(struct sockaddr *)&clientaddr,sizeof(clientaddr)))<0) {
					perror("client-sendto error");
					exit(1);
				}
				#ifdef DEBUG
					printf("Sent Groups: %s\n",groupNames);
				#endif
						
		}
		else if (messageType == 'J') {
			//join
			char inbuffer[4096];
			memset((char*)&inbuffer,0,sizeof(inbuffer));
			if((ibytes=recvfrom(sockfd,inbuffer,sizeof(inbuffer),0,(struct sockaddr *)&clientaddr,&len))==-1) {
				perror("Client-recvfrom() error");
				exit(1);
			}
			#ifdef DEBUG
				printf("Receive: %s\n",inbuffer);
			#endif
			char *pch = strtok(inbuffer,":");
			char *groupName;
			strcpy(groupName,pch);
			char *userName;
			pch = strtok(NULL,":");
			strcpy(userName,pch);
			#ifdef DEBUG
				printf("Join: group name is %s, username is %s\n",groupName,userName);
			#endif
			char messageSuccess='S';
			if(strlen(groupName)==0 || strlen(userName)==0) {
				messageSuccess='F';
			}
			else {
			//check if group name exists
			size_t i;
			int foundGroup = 0; //boolean 0 if the group is not found, 1 if it is
			int groupIndex; //the index of what group the member should be added to
			member newMember;
			strcpy(newMember.name,userName);
			strcpy(newMember.ipAddress,inet_ntoa(clientaddr.sin_addr));
			for(i=0;i<activeGroups.size();i++) {
				//success 
				if(strcmp(groupName,activeGroups[i].groupName)==0) {
					foundGroup = 1;
					groupIndex = i;
					break;
				}
				
				
			}
			//send the message success status
			if((obytes=sendto(sockfd,&messageSuccess,1,0,(struct sockaddr *)&clientaddr,sizeof(clientaddr)))<0) {
				perror("client-sendto error");
				exit(1);
			}
			if(foundGroup) {
				//add the newMember to the group
				activeGroups[groupIndex].members.push_back(newMember);
			}else {
				///create a new group
				group newGroup;
				strcpy(newGroup.groupName,groupName);
				newGroup.members.push_back(newMember);	

			}
			}//ends else when group and username actually have a length
		} //ends if message is join
			
		if(close(sockfd) !=0) {
			perror("client-sockfd closing failed");
			exit(1);
		}
	}
}
