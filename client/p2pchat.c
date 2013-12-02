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
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <sys/time.h>
#include <time.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#define DEBUG

void writeErrorLog(FILE *fpError,char *message);

int main(int argc, char **argv)
{
	if(argc!=2)
	{
		printf("usage: ./p2pchat <IP address>\n");
		exit(1);
	}

	char errorLogFile[1024] = "./logs/p2p_error_log";

	FILE *fpError = fopen(errorLogFile,"a");
	if(fpError==NULL)
	{
		printf("Error trying to open error file\n");
		exit(1);
	}

	int sockfd, n;
	struct sockaddr_in servaddr;
	
	//ip address where server is
	char* ap_addr = argv[1];
	//char *ap_addr = "123.4.5.6";
	char* port = "9768";

	//create the socket
    sockfd=socket(AF_INET,SOCK_STREAM,0);
    if(sockfd < 0) {
		writeErrorLog(fpError,"Error opening socket");
		perror("ERROR opening socket");
		exit(1);
    }

	//build server inet address
    bzero(&servaddr,sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr=inet_addr(ap_addr); //the address
    servaddr.sin_port=htons(atoi(port)); //the port

	//connect
	/*
    if(connect(sockfd,(struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
	writeErrorLog(fpError,"Error connecting");
        perror("ERROR connecting");
	exit(1);
    }

	*/
	
	close(sockfd);
	fclose(fpError);
	return 0;
} //end main

void writeErrorLog(FILE *fpError,char *message)
{
	char errorLogDate[32];
	time_t rawtime;
	struct tm *tm;	
	time(&rawtime);
	tm = localtime(&rawtime);
	strftime(errorLogDate,32,"%Y %m %d %H %M",tm);
	fprintf(fpError,"%s %s\n",errorLogDate,message);

}
