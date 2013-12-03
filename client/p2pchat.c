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
	int n= recvfrom(sockfd,recvline,2,0,NULL,NULL); //this should be one byte char
	if(n<0) {
		perror("ERROR connecting");
		exit(1);
	}

#ifdef DEBUG
	printf("%s\n",recvline);
#endif
	if(strcmp(recvline, "G")==0){ 
		while(1) {
			bzero(&recvline,sizeof(recvline));
		
			//receive actual message	
			if(recvfrom(sockfd,recvline, 1024,0,NULL,NULL) < 0) {
				perror("ERROR connecting");
				exit(1);
			}

			if(strcmp(recvline, "::")==0) {
				printf("No currently available groups\n");	
				break;
			}

			printf("%s\n",recvline); //otherwise print out the group name
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

	char sendline[1024];

	while(1) {

		printf("Command: ");
		scanf("%s",sendline);

		if(strcmp(sendline, "list") == 0) {
			requestList(sockfd, &servaddr, sizeof(servaddr));
		}
		else if(strcmp(sendline, "join")){
			//- join the group "groupname" as the user "username". 
			//Restrict both of these to single word names with printable characters.
		}
		else if(strcmp(sendline, "leave")){
			//leave - leave the group, closing all group connections. the user could optionally join 
			//another group.
		}
		else if(strcmp(sendline, "quit")){
			//- exit the program, closing all group connections.
		}
		else {
			printf("Not a valid command\n");
		}
	}

	
	close(sockfd);
	return 0;
} //end main


