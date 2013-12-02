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

//#define DEBUG

using namespace std;

pthread_mutex_t server_mutex = PTHREAD_MUTEX_INITIALIZER;

//holds data passed from main to thread
typedef struct {
	int connfd;
	struct sockaddr_in cliaddr;
	socklen_t clilen;

} thread_info;



//function run by thread upon accepting a new client connection
void *accept_client(void *input) {

	thread_info* info = (thread_info*)input;
	#ifdef DEBUG
	printf("New client accepted!\n");
	printf("\tNew client address:%s\n",inet_ntoa(info->cliaddr.sin_addr));
	printf("\tClient port:%d\n",info->cliaddr.sin_port);
	#endif
	////////////////////////////////////
	// Receive Server reply
	//



	close(info->connfd);

	return 0;
}

int main(int argc, char**argv)
{
	////////////////////////////////////
	// Setup and Connection 
	// 
	int sockfd;
	struct sockaddr_in servaddr;

	//create the socket
	sockfd=socket(AF_INET,SOCK_STREAM,0);
	if(sockfd < 0) {
		perror("ERROR opening socket");
		exit(1);
	}

	//build server inet address
	bzero(&servaddr,sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
	int s_port = 9768;
	servaddr.sin_port=htons(s_port);

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
	if(	listen(sockfd,1024) <0 ) {
		perror("listen failed");
		exit(1);
	}

	////////////////////////////////////
	// Listen to Port
	// 
	
	while(1) {
		thread_info info;
		info.clilen = sizeof(info.cliaddr);
		info.connfd = accept(sockfd,(struct sockaddr *)&info.cliaddr,&info.clilen);

		//spawn a new thread to handle the connection
		pthread_t thread;
		int status = pthread_create(&thread, NULL, accept_client, &info);

		if(status){
			#ifdef DEBUG 
			printf("error creating thread: %i\n", status);
			#endif
			exit(1);	
		}
		else{
			#ifdef DEBUG
			printf("Thread created\n");
			#endif
		}
	}
}
