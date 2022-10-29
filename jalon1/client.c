#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
 #include <err.h>
#include "common.h"

//Read from socket the size of data sent by server

int read_int_size(int fd) {
	int size_read = 0;
	int to_read = 0, offset = 0;
	while (offset != sizeof(int)) {
		to_read = read(fd, (void *)&size_read + offset, sizeof(int) - offset);
		if (-1 == to_read)
			perror("Reading size");
		if (0 == to_read) {
			printf("Should close connection, read 0 bytes\n");
			close(fd);
			return -1;
		}
		offset += to_read;
	}
	return size_read;
}

// //Write in socket the size of data to be sent

void write_int_size(int fd, void *buff) {
	int to_write = 0, offset = 0;
	while (offset != sizeof(int)) {
		to_write = write(fd, buff + offset, sizeof(int) - offset);
		if (-1 == to_write)
			perror("Writing size");
		offset += to_write;
	}
}

// Write in socket data to be sent to the server

int write_in_socket(int sockfd,char*buff,int size){

    int n;
   
    printf("Message: ");
    n = 0;
	while ((buff[n++] = getchar()) != '\n') {} // pour stocker la chaîne écrite au terminal dans buff

	//quit
	const char*s = "/quit\n";
	if(strcmp(buff,s)==0){
		
		close(sockfd);
		
	}
    int offset = 0;
    int to_write = 0;
    while(offset != size ){
		if ((to_write=write(sockfd, buff + offset, size - offset)) == -1) {
			perror("Sending to client from socket\n");
            exit(EXIT_FAILURE);
		}
    	offset+=to_write;
    }

    printf("Message sent!\n");
	return offset;
}

//Read from socket data sent by the server

int read_from_socket(int sockfd,char*buff,int size){
    
    int offset=0;
    int to_read=0;
    while(offset != size){
		to_read=read(sockfd, buff + offset, size - offset);
		if (to_read == -1) {
			perror("Receiving from server\n");
            exit(EXIT_FAILURE);
		}
		if (0 == to_read) {
			printf("Should close connection, read 0 bytes\n");
			close(sockfd);
			return -1;
			break;
		}
        offset+=to_read;
    }
	
	return offset;

}

int handle_and_connect(char *hostname, char *port) {
	int sock_fd = -1;
	// Creation of the socket
	if (-1 == (sock_fd = socket(AF_INET, SOCK_STREAM, 0))) {
		perror("Socket");
		exit(EXIT_FAILURE);
	}
	printf("Socket created successfully (%d)\n", sock_fd);
	struct addrinfo hints, *result, *rp;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_NUMERICSERV;
	if (getaddrinfo(hostname, port, &hints, &result) != 0) {
		perror("getaddrinfo");
		exit(EXIT_FAILURE);
	}
	rp = result;
	while (rp != NULL) {
		if (rp->ai_addr->sa_family == AF_INET) {
			struct sockaddr_in *sockin_ptr = (struct sockaddr_in *)rp->ai_addr;
			u_short port_number = sockin_ptr->sin_port;
			char *ip_str = inet_ntoa(sockin_ptr->sin_addr);
			printf("Address is %s:%hu\n", ip_str, htons(port_number));
			printf("Connecting...");
			//fflush(stdout);
			if (-1 == connect(sock_fd, rp->ai_addr, rp->ai_addrlen)) {
				perror("Connect");
				exit(EXIT_FAILURE);
			}
			printf("Connection OK\n");
			return sock_fd;
		}
		rp = rp->ai_next;
	}
	return -1;
}

int main(int argc, char*argv[]){
    if (argc != 3) {
		printf("Usage: ./client hostname port_number\n");
		exit(EXIT_FAILURE);
	}
	char *hostname = argv[1];
	char *port = argv[2];
    int sockfd = -1;
	if (-1 == (sockfd = handle_and_connect(hostname, port))) {
		printf("Could not create socket and connect properly\n");
		
	}

    while(1){
        char buff[MSG_LEN]={};
        int size = MSG_LEN;
       // Send data to server
       	write_int_size(sockfd, (void *)&size);
		write_in_socket(sockfd, buff, size);
       
       //Receive data from server
	   	int size2=0;
		size2 = read_int_size(sockfd);
		//alloue la mémoire
		char*str=malloc(size2);
       	read_from_socket(sockfd,str,size);
	   
	   	printf("Received from server: %s\n",str);
		//Libère la mémoire
	   	free(str);

    }
    return 0;
}
