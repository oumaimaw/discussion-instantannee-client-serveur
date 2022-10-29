#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <poll.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
 #include <err.h>
#include "common.h"

// Je me suis inspirée du code de Monsieur Joachim pour la réalisation de mon code



struct clients
{
    char *addresse;
	int port;
	int fd;
    struct clients *nxt;
};

//Fonction qui affiche les informations sur le client
void afficherClient(struct clients*liste){ 
  struct clients * tmp;
  tmp=liste;           
    while(tmp != NULL)          
        {
          printf("Le client de fd : %d a l'addresse : %s et le port %d\n",tmp->fd,tmp->addresse,tmp->port );
          tmp = tmp->nxt;     
        }
}

//Fonction qui ajoute un client à la liste chaînée
struct clients* ajouterClient(struct clients*client, int fd, int port, char *addresse){
  struct clients* NewClient = malloc(sizeof(struct clients));  // creer le Nouveau elt
  NewClient->port=port;
  NewClient->addresse=addresse; 
  NewClient->fd=fd;                        
  NewClient->nxt=client;                          
  return NewClient;                               
}

int read_int_size(int fd) {
	int read_value = 0;
	int ret = 0, offset = 0;
	while (offset != sizeof(int)) {
		ret = read(fd, (void *)&read_value + offset, sizeof(int) - offset);
		if (-1 == ret){
			perror("Reading size");
			break;
		}
		if (0 == ret) {
			printf("Should close connection, read 0 bytes\n");
			close(fd);
			return -1;
		}
		offset += ret;
	}
	return read_value;
}

void write_int_size(int fd, void *ptr) {
	int ret = 0, offset = 0;
	while (offset != sizeof(int)) {
		ret = write(fd, ptr + offset, sizeof(int) - offset);
		if (-1 == ret){
			perror("Writing size");
			break;
		}
		offset += ret;
	}
}


int read_from_socket(int sockfd, char * buff, int size){
    int ret = 0;
	int offset = 0;
	while (offset != size) {
		ret = read(sockfd, buff + offset, size - offset);
		if (-1 == ret) {
			perror("Reading from client socket");
			exit(EXIT_FAILURE);
		}
		if (0 == ret) {
			printf("Should close connection, read 0 bytes\n");
			close(sockfd);
			return -1;
			break;
		}
		offset += ret;
	}
	

	return offset;
}

int write_in_socket(int sockfd,char* buf, int size){
   int ret = 0, offset = 0;
	while (offset != size) {
		if (-1 == (ret = write(sockfd, buf + offset, size - offset))) {
			perror("Writing from client socket");
			return -1;
		}
		offset += ret;
	}
	return offset;
}   

//fonction qui crée une socket d'écoute et gère le bind avec le client

int listen_and_bind(char*port){

    //Création de la socket d'écoute

    int listen_fd = -1;
	if (-1 == (listen_fd = socket(AF_INET, SOCK_STREAM, 0))) {
		perror("Socket");
		exit(EXIT_FAILURE);
	}
    printf("Listen socket descriptor %d\n", listen_fd);

    int yes = 1;
	if (-1 == setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))) {
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}

    struct addrinfo hints, *result, *rp;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE|AI_NUMERICSERV;
	if (getaddrinfo(NULL, SERV_PORT, &hints, &result) != 0) {
		perror("getaddrinfo()");
		exit(EXIT_FAILURE);
	}
    rp = result;
	while (rp != NULL) {
		if (rp->ai_family == AF_INET) {
			struct sockaddr_in *sockptr = (struct sockaddr_in *)(rp->ai_addr);
			struct in_addr local_address = sockptr->sin_addr;
			printf("Binding to %s on port %hd\n",
						 inet_ntoa(local_address),
						 ntohs(sockptr->sin_port));

			if (-1 == bind(listen_fd, rp->ai_addr, rp->ai_addrlen)) {
				perror("Binding");
			}
			if (-1 == listen(listen_fd, 30)) {
				perror("Listen");
			}
			return listen_fd;
		}
		rp = rp->ai_next;
	}
	return listen_fd;
}

void server(int listen_fd){
    // Declare array of struct pollfd
	int nfds = 10;
	struct pollfd pollfds[nfds];
	struct clients*liste_chainee=NULL;
	// Init first slot with listening socket
	pollfds[0].fd = listen_fd;
	pollfds[0].events = POLLIN;
	pollfds[0].revents = 0;
	// Init remaining slot to default values
	for (int i = 1; i < nfds; i++) {
		pollfds[i].fd = -1;
		pollfds[i].events = 0;
		pollfds[i].revents = 0;
	}

	// server loop
	while (1) {

		// Attend la détection d'une activité dans la socket
		int n_active = 0;
		if (-1 == (n_active = poll(pollfds, nfds, -1))) {
			perror("Poll");
		}
		
		for (int i = 0; i < nfds; i++) {
			

			// Accepte la connexion si la socket d'ecoute est active
			if (pollfds[i].fd == listen_fd && pollfds[i].revents & POLLIN) {
				
				struct sockaddr client_addr;
				socklen_t size = sizeof(client_addr);
				int client_fd;
				if (-1 == (client_fd = accept(listen_fd, &client_addr, &size))) {
					perror("Accept");
				}

				// affiche les informations concernant le client
				struct sockaddr_in *sockptr = (struct sockaddr_in *)(&client_addr);
				struct in_addr client_address = sockptr->sin_addr;
				printf("Connection succeeded and client used %s:%hu \n", inet_ntoa(client_address), ntohs(sockptr->sin_port));
				printf("client_fd = %d\n", client_fd);
				liste_chainee=ajouterClient(NULL, client_fd, ntohs(sockptr->sin_port), inet_ntoa(client_address));
				afficherClient(liste_chainee);
				// stocke le nouveau fd dans le tableau pollfds
				for (int j = 0; j < nfds; j++) {
					if (pollfds[j].fd == -1) {
						pollfds[j].fd = client_fd;
						pollfds[j].events = POLLIN;
						break;
					}
				}

				pollfds[i].revents = 0;
			}else if (pollfds[i].fd != listen_fd && pollfds[i].revents & POLLHUP) { // If a socket previously created to communicate with a client detects a disconnection from the client side
				
				printf("client on socket %d has disconnected from server\n", pollfds[i].fd);
				
				close(pollfds[i].fd);
				pollfds[i].events = 0;
				pollfds[i].revents = 0;
			 }
			//Si une socket différente de celle de l'écoute est active
			else if (pollfds[i].fd != listen_fd && pollfds[i].revents & POLLIN) { 
				
				int size = 0;
				size = read_int_size(pollfds[i].fd);
				//allocation dynamique de la mémoire
				char *str = malloc(size);
				//Read data from socket
                read_from_socket(pollfds[i].fd,str,size);
				printf("Client on socket %d says: %s\n",pollfds[i].fd,str);

				// send back data to client

                write_int_size(pollfds[i].fd, (void *)&size);
				write_in_socket(pollfds[i].fd, str, size);
				//Libère la mémoire
				free(str);
				
				pollfds[i].revents = 0;
			}
			 
			 
			
		}
	}
}
  

int main(int argc, char*argv[]){
    
	if (argc != 2) {
		printf("Usage: ./server port_number\n");
		exit(EXIT_FAILURE);
	}

	char *port = argv[1];
	int listen_fd = -1;
	if (-1 == (listen_fd = listen_and_bind(port))) {
		printf("Could not create, bind and listen properly\n");
		return 1;
	}
	
	server(listen_fd);
	
	return 0;
}

