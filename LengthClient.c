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
#include <sys/wait.h>
#include <arpa/inet.h>
#include <signal.h>
#include <stdint.h>

//#define PORT "10010" // the port client will be connecting to 

#define MAXDATASIZE 1024 // max message size
#define BACKLOG 10	 // how many pending connections queue will hold

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void sigchld_handler(int s)
{
	// waitpid() might overwrite errno, so we save and restore it:
	int saved_errno = errno;

	while(waitpid(-1, NULL, WNOHANG) > 0);

	errno = saved_errno;
}

void addShort(unsigned char buf[], int startPoint, unsigned short numToAdd);
unsigned short readShort(unsigned char buf[], int startPoint);
void addLong(unsigned char buf[], int startPoint, unsigned long numToAdd);
unsigned long readLong(unsigned char buf[], int startPoint);

int main(int argc, char *argv[])
{
	int sockfd, numbytes;  
	unsigned char buf[MAXDATASIZE];
	struct addrinfo hints, *servinfo, *p;
	int rv;
	socklen_t sin_size;
	struct sigaction sa;
	char s[INET6_ADDRSTRLEN];
	struct sockaddr_storage their_addr;
	socklen_t addr_len;
	int yes = 1;

	if (argc != 4) {
	    fprintf(stderr,"usage: Client ServerName ServerPort MyPort\n");
	    exit(1);
	}

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	
	char* serverName = argv[1];
	char* serverPort = argv[2];
	unsigned short myPort = atoi(argv[3]);
	
	if (myPort < 10075 || myPort > 10079) {
		fprintf(stderr, "myPort must be between within the range (10075, 10079) inclusive");
		exit(1);
	}
	
	if ((rv = getaddrinfo(serverName, serverPort, &hints, &servinfo)) != 0) {
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

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "client: failed to create socket\n");
		return 2;
	}

	unsigned long magicNum = 0x4A6F7921; 
    unsigned char GID = 13; 
	addLong(buf, 0, magicNum);
	addShort(buf, 4, myPort);
	buf[6] = GID;
	buf[7] = GID; //Increases size
	
	if ((numbytes =  sendto(sockfd, buf, 7, 0, p->ai_addr, p->ai_addrlen)) == -1) {
		perror("talker: sendto");
		exit(1);
	}


	printf("Waiting for response\n");
	addr_len = sizeof their_addr;
	if   ((numbytes =  recvfrom(sockfd, buf, 11 , 0,(struct sockaddr *)&their_addr, &addr_len)) == -1) {
		perror("recvfrom");
		exit(1);
	}
	
	printf("Response received\n");
	//printf("Num bytes: %d\n", numbytes);
	if (numbytes != 7 && numbytes != 11) {
		perror("Invalid response from server\n");
		close(sockfd);
		exit(1);
	}
	
	
	//Done talking to server
	close(sockfd);
	
	//Form a TCP socket
	int new_fd;
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP
	
	
	if (numbytes == 7) {
		unsigned long returnedMagicNum = readLong(buf, 0);
		unsigned char returnedGID = buf[4];
		unsigned short returnedPortNum = readShort(buf, 5);
		if (returnedPortNum < 10010) {
			perror("Invalid frame sent to server.");
		    exit(1);
		}
		
		//You are the waiting client. 
		//Form a TCP server and wait for response from next client	

		if ((rv = getaddrinfo(NULL, argv[3], &hints, &servinfo)) != 0) {
			fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
			return 1;
		}

		// loop through all the results and bind to the first we can
		for(p = servinfo; p != NULL; p = p->ai_next) {
			if ((sockfd = socket(p->ai_family, p->ai_socktype,
					p->ai_protocol)) == -1) {
				perror("server: socket");
				continue;
			}

			if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
					sizeof(int)) == -1) {
				perror("setsockopt");
				exit(1);
			}

			if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
				close(sockfd);
				perror("server: bind");
				continue;
			}

			break;
		}

		freeaddrinfo(servinfo); // all done with this structure

		if (p == NULL)  {
			fprintf(stderr, "server: failed to bind\n");
			exit(1);
		}

		if (listen(sockfd, BACKLOG) == -1) {
			perror("listen");
			exit(1);
		}

		sa.sa_handler = sigchld_handler; // reap all dead processes
		sigemptyset(&sa.sa_mask);
		sa.sa_flags = SA_RESTART;
		if (sigaction(SIGCHLD, &sa, NULL) == -1) {
			perror("sigaction");
			exit(1);
		}

		printf("status: waiting for a partner to connect.\n");
	
		sin_size = sizeof their_addr;
		new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
		if (new_fd == -1) {
			perror("accept");
			//continue;
		}

		inet_ntop(their_addr.ss_family,
		get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);
		printf("status: got connection from %s\n", s);
		
		while(1) {
			//wait to receive message, display it, then prompt user for response.
			
			//clear buffer
			memset(buf, 0, sizeof buf);
			
			if ((numbytes = recv(new_fd, buf, MAXDATASIZE-1, 0)) == -1) 
			{
				perror("recv");
				exit(1);
			}
			printf("Received: %s", buf);
			
			//Read input
			printf("Enter your message: ");
			char message[MAXDATASIZE];
			fgets(message, MAXDATASIZE, stdin);
			
			if (send(new_fd, message, strlen(message), 0) == -1) 
			{
				perror("send");
				exit(1); 
			}
			printf("Waiting for response...\n");
		}
		
	}
	else {
		//Time to connect to waiting client
		unsigned long returnedMagicNum = readLong(buf, 0);
		unsigned long waitingClientIP = readLong(buf, 4);
		unsigned short returnedPortNum = readShort(buf, 8);
		unsigned char returnedGID = buf[10];

		//Convert waiting client's port num to string
		char waitingPort[6];
		sprintf(waitingPort, "%d", returnedPortNum);

		waitingClientIP = ntohl(waitingClientIP);
		char str[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &(waitingClientIP), str, INET_ADDRSTRLEN);

		if ((rv = getaddrinfo(str, waitingPort, &hints, &servinfo)) != 0) {
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

			if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
				close(sockfd);
				perror("client: connect");
				continue;
			}
			break;
		}

		if (p == NULL) {
			fprintf(stderr, "client: failed to connect\n");
			return 2;
		}

		inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
		printf("client: connecting to %s\n", s);
		
		while(1) {
			//wait to receive message, display it, then prompt user for response.
			
			//clear buffer
			memset(buf, 0, sizeof buf);
			
			//Read input
			printf("Enter your message: ");
			char message[MAXDATASIZE];
			fgets(message, MAXDATASIZE, stdin);
			
			if (send(sockfd, message, strlen(message), 0) == -1) 
			{
				perror("send");
				exit(1); 
			}
			
			printf("Waiting for response...\n");
			
			if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) 
			{
				perror("recv");
				exit(1);
			}
			buf[numbytes] = '\0';
			printf("Received: %s", buf);
		}	
	}
	
	freeaddrinfo(servinfo); // all done with this structure
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
void addLong(unsigned char buf[], int startPoint, unsigned long numToAdd) {
	buf[startPoint] = numToAdd >> 24;
	buf[startPoint + 1] = numToAdd >> 16;
	buf[startPoint + 2] = numToAdd >> 8;
	buf[startPoint + 3] = numToAdd;
}

//Reads long from buf in host byte order starting from index startPoint
unsigned long readLong(unsigned char buf[], int startPoint) {
	unsigned long numIWant = buf[startPoint] << 24 | buf[startPoint + 1] << 16 | buf[startPoint + 2] << 8 | buf[startPoint + 3];
}
