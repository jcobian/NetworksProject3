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
	char messageType[2];


	//////////////////////////////////////
	//main loop listening for incoming UDP packets
	//
	vector<group> activeGroups;	

	while(1) { 
		memset((char*)&messageType,0,sizeof(messageType));

		bzero(&clientaddr, len);

		if((recvfrom(sockfd,messageType,sizeof(messageType),0,(struct sockaddr *)&clientaddr,&len) < 0)) {
			perror("Client-recvfrom() error");
			exit(1);
		}
		//////////////////////////////////
		//list
		if(strcmp(messageType,"L")==0) {

			if(sendto(sockfd,"G",strlen("G"),0,(struct sockaddr *)&clientaddr,sizeof(clientaddr))<0) {
				perror("client-sendto error");
				exit(1);
			}
		
			if(activeGroups.size()==0) {
				#ifdef DEBUG
					printf("Active groups is empty\n");
				#endif
				char colons[3];
				strcpy(colons,"::");
				if(sendto(sockfd,colons,strlen(colons),0,(struct sockaddr *)&clientaddr,sizeof(clientaddr))<0) {
					perror("client-sendto error");
					exit(1);
				}
			} else {
				string tempName="";
				#ifdef DEBUG
					printf("Sending group names:\n");
				#endif

				for(size_t i=0;i<activeGroups.size();i++) {
					tempName+=activeGroups[i].groupName;
					tempName+=":";
					if(i==activeGroups.size()-1) {
						tempName+=":";
					}
				}
				#ifdef DEBUG
					cout<<tempName<<endl;
				#endif

				if(sendto(sockfd,tempName.c_str(),strlen(tempName.c_str()),0,(struct sockaddr *)&clientaddr,sizeof(clientaddr))<0) {
					perror("client-sendto error");
					exit(1);
				}
			}
					
		}
		//////////////////////////////////
		//join
		else if (strcmp(messageType,"J")==0) {
			char inbuffer[1024];
			memset((char*)&inbuffer,0,sizeof(inbuffer));
			if(recvfrom(sockfd,inbuffer,sizeof(inbuffer),0,(struct sockaddr *)&clientaddr,&len)<0) {
				perror("Client-recvfrom() error");
				exit(1);
			}

			#ifdef DEBUG
				printf("GroupUser combo Receive: %s\n",inbuffer);
			#endif

			string groupName="",userName="";
			string line = inbuffer;
			string s;
			stringstream ss(line);
			getline(ss,s,':');
			groupName = s;
			getline(ss,s,':');
			userName = s;
			
			#ifdef DEBUG
				cout<<"Join: group name "<<groupName<< ", username "<<userName<<endl;
			#endif
			string messageSuccess="S";
			if(groupName.length()==0 || userName.length()==0) { //no groupsname or username was received
				messageSuccess="F";
			}
			//send the message success status
			if(sendto(sockfd,messageSuccess.c_str(),strlen(messageSuccess.c_str()),0,(struct sockaddr *)&clientaddr,sizeof(clientaddr))<0) {
				perror("client-sendto error");
				exit(1);
			}
			#ifdef DEBUG
				cout<<"Sending message success of "<<messageSuccess<<endl;
			#endif


			//we succesfully recieved the groupname and username, so now attempt to add them to our list
			if(messageSuccess=="S") {
				int foundGroup = 0; //boolean 0 if the group is not found, 1 if it is
				int groupIndex; //the index of what group the member should be added to
				member newMember;
				newMember.name = userName;
				newMember.ipAddress = inet_ntoa(clientaddr.sin_addr);

				for(size_t i=0;i<activeGroups.size();i++) { //success 
					if(groupName==activeGroups[i].groupName) {	
						foundGroup = 1;
						groupIndex = i;
						break;
					}
				}

				string result="";
				if(foundGroup) {
					string username="",address="";
					//turn the current members into a string to be sent to client
					for(unsigned int i=0;i<activeGroups[groupIndex].members.size();i++) {
						username = activeGroups[groupIndex].members[i].name;
						username+=":";
						cout << "username: " << username<<endl;
						address = activeGroups[groupIndex].members[i].ipAddress;
						address+=":";
						cout << "address: " << address<<endl;
						result += (username + address);
					}

					cout<<"Found group and users are "<<result<<endl;
					result+=":";

					//add the newMember to the group
					activeGroups[groupIndex].members.push_back(newMember);
				} else {
					cout<<"Creating new group "<<groupName<<" with "<<newMember.name<<endl;
					///create a new group
					group newGroup;
					newGroup.groupName = groupName;
					newGroup.members.push_back(newMember);	
					/*
					result+=newMember.name;
					result+=":";
					result+=newMember.ipAddress;
					*/
					result+="::";
					activeGroups.push_back(newGroup);		
				}
				if(sendto(sockfd,result.c_str(),strlen(result.c_str()),0,(struct sockaddr*)&clientaddr,sizeof(clientaddr))<0){
						perror("client send to error");
						exit(1);	
				}
				#ifdef DEBUG
					cout<<"Sending members of group "<<result<<endl;
				#endif
					
			}//ends else when group and username actually have a length
		} //ends if message is join
			
/*		if(close(sockfd) !=0) {
			perror("client-sockfd closing failed");
			exit(1);
		}*/
	}
}
