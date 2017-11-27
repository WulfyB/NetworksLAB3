/*
** Client.c -- Lab3
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <arpa/inet.h>

//#define PORT "10010" // the port client will be connecting to 

#define MAXDATASIZE 1024 // max message size

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void addShort(unsigned char buf[], int startPoint, unsigned short numToAdd);
unsigned short readShort(unsigned char buf[], int startPoint);
void addLong(unsigned char buf[], int startPoint, unsigned short numToAdd);
unsigned long readLong(unsigned char buf[], int startPoint);

int main(int argc, char *argv[])
{
	int sockfd, numbytes;  
	unsigned char buf[MAXDATASIZE];
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET6_ADDRSTRLEN];

	if (argc != 2) {
	    fprintf(stderr,"usage: Client ServerName ServerPort MyPort\n");
	    exit(1);
	}

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	
	char* serverName = argv[1];
	char* portNum = argv[2];
	short myPort = atoi(argv[3]);
	
	if (myPort < 10075 || myPort > 10079) {
		fprintf(stderr, "myPort must be between within the range (10075, 10079) inclusive");
		exit(1);
	}
	
	if ((rv = getaddrinfo(serverName, portNum, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and connect to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}

		//Use this for TCP stuff later
		//if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
		//	close(sockfd);
		//	perror("client: connect");
		//	continue;
		//}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "client: failed to create socket\n");
		return 2;
	}

	//Not necessary, but useful
	//inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
	//		s, sizeof s);
	//printf("client: connecting to %s\n", s);

	long magicNum = 0x4A6F7921;
    unsigned char GID = 13;
	addLong(buf, 0, magicNum);
	addShort(buf, 4, myPort);
	buf[6] = GID;

	if ((numbytes =  sendto(sockfd, buf, 7, 0, 
		p->ai_addr, p->ai_addrlen)) ==   -1) {
		perror("talker: sendto");
		exit(1);
		}
		
		
	//Now to get response
	freeaddrinfo(servinfo); // all done with this structure

	//Use this for TCP stuff later
	//if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
	//    perror("recv");
	//    exit(1);
	//}

	buf[numbytes] = '\0';

	printf("client: received '%s'\n",buf);

	close(sockfd);

	return 0;
}

//Adds short to buf in network byte order starting from index startPoint
void addShort(unsigned char buf[], int startPoint, unsigned short numToAdd) {
	buf[startPoint] = numToAdd >> 8;
	buf[startPoint + 1] = numToAdd;
}

//Reads short from buf in host byte order starting from index startPoint
unsigned short readShort(unsigned char buf[], int startPoint) {
	unsigned short numIWant = buf[startPoint] << 8 | buf[startPoint + 1];
	return numIWant;
}

//Adds long to buf in network byte order starting from index startPoint
void addLong(unsigned char buf[], int startPoint, unsigned short numToAdd) {
	buf[startPoint] = numToAdd >> 24;
	buf[startPoint + 1] = numToAdd >> 16;
	buf[startPoint + 2] = numToAdd >> 8;
	buf[startPoint + 3] = numToAdd;
}

//Reads long from buf in host byte order starting from index startPoint
unsigned long readLong(unsigned char buf[], int startPoint) {
	unsigned long numIWant = buf[startPoint] << 24 | buf[startPoint + 1] << 16 | buf[startPoint + 2] << 8 | buf[startPoint + 3];
}