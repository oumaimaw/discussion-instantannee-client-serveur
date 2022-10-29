#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
 #include <ctype.h>
 #include <poll.h>
#include "common.h"
#include "msg_struct.h"

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
	printf("la size recu est %d\n",to_read);
	return size_read;
}

// //Write in socket the size of data to be sent

void write_int_size(int fd, void *buff) {
	int to_write = 0, offset = 0;
	while (offset != sizeof(int)) {
		to_write = write(fd, buff + offset, sizeof(int) - offset);
		if (-1 == to_write ){	
			perror("Writing size");
			break;
		}
		offset += to_write;
	}
}

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

int write_in_socket(int fd, void *buf, int size) {

	int ret = 0, offset = 0;
	while (offset != size) {
		if (-1 == (ret = write(fd, buf + offset, size - offset))) {
			perror("Writing from client socket");
			return -1;
		}
		offset += ret;
	}
	return offset;
}

void echo_client(int sockfd){
	struct message msgstruct;
	char buff[MSG_LEN];
	char to_send[MSG_LEN];    //stocke le message à envoyer
	char nickname[NICK_LEN];  //stocke le client à qui on envoie le message dans /msg
	memset(nickname,0,NICK_LEN);
	memset(&msgstruct,0,sizeof(msgstruct));
	char request[MSG_LEN];    //stocke la requête initiale passée par le client
	char client_and_msg[MSG_LEN]; //stocke le client et le message à envoyer pour /msg
	char message[MSG_LEN];     //stocke que le message à envoyer pour /msg
	memset(request,0,MSG_LEN);
	memset(client_and_msg,0,MSG_LEN);
	memset(message,0,MSG_LEN);
	
	int size;
	int n;
	

	// Declare array of struct pollfd
	int nfds = 2;
	struct pollfd pollfds[nfds];
	memset(pollfds,0,sizeof(struct pollfd));

	//stdin_fileno va gérer l'écriture dans la socket
	pollfds[0].fd=STDIN_FILENO;
	pollfds[0].events = POLLIN;
	pollfds[0].revents = 0;

	// permettra de gérer la lecture des données envoyées par le serveur
	pollfds[1].fd = sockfd;
	pollfds[1].events = POLLIN;
	pollfds[1].revents = 0;
	size = read_int_size(sockfd);
	char*login_msg= malloc(size);
	read_from_socket(sockfd, login_msg, size);		
	fflush(stdout);
		
	while(1){
		
		// Attend la détection d'une activité dans la socket
		int n_active = 0;
		if (-1 == (n_active = poll(pollfds, nfds, -1))) {
			perror("Poll");
		}
		
		
		if(pollfds[0].revents == POLLIN){
		
		

		memset(buff, 0, MSG_LEN);
		memset(to_send, 0, MSG_LEN);
		
		
    	n = 0;
		memset(buff, 0, MSG_LEN);
		memset(&msgstruct, 0, sizeof(struct message));

		

		while ((buff[n++] = getchar()) != '\n') {} // pour stocker la chaîne écrite au terminal dans buff
		message:
		strcpy(request,strtok(buff,"\n"));
		strcpy(client_and_msg,strtok(buff,"\n"));
		//Extraire l'information qu'il faut transmettre du buff en utilisant la fct strtok
		const char * separators = " ";
		char * strToken = strtok ( buff, separators );  // permet d'extraire la commande passée du client(/nick or /who...)
		//Traiter le cas /nick
		if(strcmp(strToken,"/nick")==0){
				
				strcpy(msgstruct.infos, strcpy(request, request + strlen("/nick ")));
				printf("infos = %s\n",msgstruct.infos);
   				
				
				//Tester si le nickname est correct
				int i;
				for(i=0;i<strlen(msgstruct.infos);i++){
					if(strlen(msgstruct.infos)>128 || (msgstruct.infos[i]>=32 && msgstruct.infos[i]<=47) || (msgstruct.infos[i]>=58 && msgstruct.infos[i]<=64) || (msgstruct.infos[i]>=91 && msgstruct.infos[i]<=94) || (msgstruct.infos[i]>=123 && msgstruct.infos[i]<=126)){
						printf("Pseudo incorrect,veuillez inscrire un pseudo pas long et sans caractères spéciaux et sans espace\n");
						memset(buff,0,MSG_LEN);
						n=0;
						while ((buff[n++] = getchar()) != '\n') {}
						goto message;
					}
				}
				msgstruct.pld_len = 0;
				strcpy(msgstruct.nick_sender, strcpy(request, request + strlen("/nick ")));
				msgstruct.type = NICKNAME_NEW;
				
				
		}
		//Traiter le cas /who
		else if(strcmp(strToken,"/who") == 0){
			strcpy(msgstruct.infos,"");
			msgstruct.pld_len = 0;
			msgstruct.type = NICKNAME_LIST;
		}
		
		//Traiter le cas whois
		else if(strcmp(strToken,"/whois") == 0){
			strcpy(msgstruct.infos, strcpy(request, request + strlen("/whois ")));
			msgstruct.pld_len = 0;
			msgstruct.type = NICKNAME_INFOS;
		}
	
		//Traiter le cas msgall
		else if(strcmp(strToken,"/msgall") == 0){
			
			strcpy(msgstruct.infos,"");
			
			strcpy(to_send, strcpy(request, request + strlen("/msgall ")));
			msgstruct.pld_len = strlen(to_send)+1;
			msgstruct.type = BROADCAST_SEND;
		}

		//Traiter le cas /msg 
		else if(strcmp(strToken,"/msg") == 0){
			
			strcpy(message, strcpy(client_and_msg, client_and_msg + strlen("/msg ")));
			strcpy(nickname,strtok(client_and_msg," "));  //recuperer le client a qui on envoie le message
			strcpy(msgstruct.infos,nickname);
			strcpy(to_send, strcpy(message, message + strlen(nickname))); //recuperer le message a envoyer
			msgstruct.pld_len = strlen(to_send)+1;
			msgstruct.type = UNICAST_SEND;
		}
		
		//Traiter le cas echo_send

		else {
			strcpy(to_send,request );
			msgstruct.pld_len = strlen(to_send)+1;
			strcpy(msgstruct.infos,"");
			msgstruct.type = ECHO_SEND;

		}

		printf("pld_len: %i / nick_sender: %s / type: %s / infos: %s\n", msgstruct.pld_len, msgstruct.nick_sender, msg_type_str[msgstruct.type], msgstruct.infos);
		//Send data to the server
		write_in_socket(sockfd,(void*)&msgstruct,sizeof(msgstruct));
		if(msgstruct.pld_len != 0){
			write_in_socket(sockfd,to_send,msgstruct.pld_len);	
		}
		
		pollfds[0].revents = 0;
		}

		else if(pollfds[1].revents == POLLIN){
		
		//cleaning memory
		memset(to_send,0,MSG_LEN);
		memset(buff,0,MSG_LEN);

		//reading data from socket
		
		int size = read_int_size(sockfd);
		
		read_from_socket(sockfd,buff,size);
		fflush(stdout);
		pollfds[1].revents = 0;
		
		}
	}
	
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
			fflush(stdout);
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

int main(int argc, char*argv[]) {
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
	echo_client(sockfd);
    return 0;

}










