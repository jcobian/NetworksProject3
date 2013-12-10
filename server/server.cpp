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
#include <map>
#include <vector>

#include <pthread.h>

//#define DEBUG

using namespace std;


map<string, vector<member> > activeGroups;	//key is the name of the group, value is a vector of members

//runs on a seperate thread, every so often check all users, and if any haven't checked in with a ping
//in some amount of time, drop them
void *dropSilent(void * input) {

	while(1) {
		usleep(120000000); //every 2 minutes
//		usleep(5200000); //every 5 seconds for testing 

		//iterate over available groups 
		for( map<string,vector<member> >::iterator it=activeGroups.begin(); it!=activeGroups.end(); ++it) {
			vector<member> v_m = it->second;
			//iterate over the users
			for(unsigned int i=0; i<v_m.size(); i++){
				#ifdef DEBUG
					cout << "possible drop of: " << v_m[i].name << " : " << v_m[i].recent_ping<<endl;
				#endif
				if(v_m[i].recent_ping+120 < time(NULL)) {//its been 2 minutes since the last ping
					#ifdef DEBUG
						cout << "removing user " << v_m[i].name << endl;
					#endif
					it->second.erase(it->second.begin()+i); //so remove this user
				}
			}

			if(it->second.empty()) { //if this group no longer has users we should probably remove it
				activeGroups.erase(it);
			}
		}
	}
	return 0;
}

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


	/////////////////////////////////////
	//spawn thread for ping dropping
	pthread_t ping_thread;
	int info;
	pthread_create(&ping_thread, NULL, dropSilent, &info);


	//////////////////////////////////////
	//main loop listening for incoming UDP packets
	//

	while(1) { 
		memset((char*)&messageType,0,sizeof(messageType));

		bzero(&clientaddr, len);

		if((recvfrom(sockfd,messageType,sizeof(messageType),0,(struct sockaddr *)&clientaddr,&len) < 0)) {
			perror("Client-recvfrom() error");
			exit(1);
		}

		#ifdef DEBUG
			cout << "Incoming Flag: " <<messageType << endl;
		#endif	

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

				//iterate over available groups and concatenate to tempName
				for( map<string,vector<member> >::iterator it=activeGroups.begin(); it!=activeGroups.end(); ++it) {
					tempName+= (*it).first+":";
				}
				tempName+=":"; //add an extra semicolon to signify the end of the list

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
				member newMember;
				newMember.name = userName;
				newMember.recent_ping = time(NULL);
				newMember.ipAddress = inet_ntoa(clientaddr.sin_addr);

				//create a new group and add the member
				activeGroups[groupName].push_back(newMember);

				string result="";
				string username="",address="";
				//turn the current members into a string to be sent to client
				for(unsigned int i=0;i<activeGroups[groupName].size();i++) {
					result += activeGroups[groupName][i].name + ":" + activeGroups[groupName][i].ipAddress + ":";
				}
				result+=":";//add additional semicolon to signify end of list

				#ifdef DEBUG
					cout<<"Found group and users are "<<result<<endl;
				#endif

				if(sendto(sockfd,result.c_str(),strlen(result.c_str()),0,(struct sockaddr*)&clientaddr,sizeof(clientaddr))<0){
						perror("client send to error");
						exit(1);	
				}
				#ifdef DEBUG
					cout<<"Sending members of group "<<result<<endl;
				#endif
					
			}//ends else when group and username actually have a length
		} 
		//////////////////////////////////
		//ping
		else if (strcmp(messageType,"P")==0) {

			//receive the group:user
			char inbuffer[1024];
			memset((char*)&inbuffer,0,sizeof(inbuffer));
			if(recvfrom(sockfd,inbuffer,sizeof(inbuffer),0,(struct sockaddr *)&clientaddr,&len)<0) {
				perror("Client-recvfrom() error");
				exit(1);
			}

			string groupName="",userName="";
			string line = inbuffer;
			string s;
			stringstream ss(line);
			getline(ss,s,':');
			groupName = s;
			getline(ss,s,':');
			userName = s;

			if(groupName.length()!=0 && userName.length()!=0) { //we succesfully recieved the groupname and username, so now attempt to remove them from the desired list

				if(activeGroups.find(groupName) != activeGroups.end()) { //if the group exists in activeGroups
					//now update recent_ping for user from the group
					for(unsigned int i=0;i<activeGroups[groupName].size();i++) {
						if(activeGroups[groupName][i].name == userName) {

							//update the recent time timestamp
							activeGroups[groupName][i].recent_ping = time(NULL);

							#ifdef DEBUG
								cout<<"Recieved ping from user: "<<userName<< " in: "<<groupName<<endl;
							#endif
						}
					}
				} else { 
					cout << "ERROR: User pinged from a group that doesn't exist?"<<endl;
				}
			}

			#ifdef DEBUG
				//now remove that user from the group
				for(unsigned int i=0;i<activeGroups[groupName].size();i++) {

					cout << activeGroups[groupName][i].name << " : "<<activeGroups[groupName][i].recent_ping << endl;

				}
			#endif
		}
		//////////////////////////////////
		//leave
		else if (strcmp(messageType,"Q")==0) {
	
			//receive the group:user
			char inbuffer[1024];
			memset((char*)&inbuffer,0,sizeof(inbuffer));
			if(recvfrom(sockfd,inbuffer,sizeof(inbuffer),0,(struct sockaddr *)&clientaddr,&len)<0) {
				perror("Client-recvfrom() error");
				exit(1);
			}

			string groupName="",userName="";
			string line = inbuffer;
			string s;
			stringstream ss(line);
			getline(ss,s,':');
			groupName = s;
			getline(ss,s,':');
			userName = s;
			
			string messageSuccess="F"; //changes to S only upon succesful removal from group

			if(groupName.length()!=0 && userName.length()!=0) { //we succesfully recieved the groupname and username, so now attempt to remove them from the desired list

				if(activeGroups.find(groupName) != activeGroups.end()) { //if the group exists in activeGroups
					//now remove that user from the group
					for(unsigned int i=0;i<activeGroups[groupName].size();i++) {
						if(activeGroups[groupName][i].name == userName) {
							activeGroups[groupName].erase(activeGroups[groupName].begin()+i);

							messageSuccess="S"; //let client know removal was succesful

							#ifdef DEBUG
								cout<<"Removed user: "<<userName<< " from: "<<groupName<<endl;
							#endif
						}
					}
				} else { //otherwise they tried to leave a group that didn't exist, ERROR
					cout << "ERROR: User wanted to leave a group that doesn't exist?"<<endl;
				}
			}
			
			//send the message success status
			if(sendto(sockfd,messageSuccess.c_str(),strlen(messageSuccess.c_str()),0,(struct sockaddr *)&clientaddr,sizeof(clientaddr))<0) {
				perror("client-sendto error");
				exit(1);
			}
		} 
		else {
			cout << "Received a message with non-recognizable flag?  lulz"<<endl;
		}
	}
}
