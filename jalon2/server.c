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
#include <time.h>

#include "common.h"
#include "msg_struct.h"


// //Fonction qui affiche les informations sur le client
void afficherClient(struct info_client*clients){ 

  struct info_client * tmp;
  tmp=clients;           
    while(tmp != NULL)          
        {
          printf("Le client : %s de socket %d a l'addresse : %s et le port %d\n",tmp->nickname,tmp->fd,tmp->addresse,tmp->port);
          tmp = tmp->next_client;     
        }
}


//Fonction qui ajoute un client à la liste chaînée
struct info_client* ajouterClient(struct info_client*client, int fd, int port,char date[],char*addresse){
  struct info_client* NewClient = malloc(sizeof(struct info_client)); 
  NewClient->fd=fd;
  NewClient->port=port;
  strcpy(NewClient->date,date); 
  NewClient->addresse=addresse; 
  strcpy(NewClient->nickname,"");                        
  NewClient->next_client=client; 
  //printf("Le client : %s de socket %d a l'addresse : %s et le port %d\n",NewClient->nickname,NewClient->fd,NewClient->addresse,NewClient->port);                      
  return NewClient;                               
}

//Fonction qui retourne le nombre de clients d'une liste chainee
int nombreClients(struct info_client*client)
{
    
    if(client == NULL)
        return 0;

    
    return nombreClients(client->next_client)+1;
}

// // Fonction qui renvoie la structure du client de pseudo nickname
struct info_client*findClient_by_pseudo(struct info_client* clients, char nickname[]){
	
  	if(clients==NULL){
		  
    	return NULL;
  	}
	
  	else{
		int compteur = 0;    //valeur qu'on va comparer avec le nombre de clients de la liste chainée pour voir si oui ou non le nick existe
    	struct info_client* tmp=clients;
    	while(tmp->next_client!=NULL){
      		if (strcmp(tmp->nickname,nickname)==0) {
        		return tmp;
			}  
			compteur=compteur+1;                  
    		tmp=tmp->next_client;
			
        }
		//Gérer le cas où le pseudo passée n'existe pas
		if (compteur+1 == nombreClients(clients) && strcmp(tmp->nickname,nickname)!=0){
			return NULL;
		}
		else
      		return tmp;
    }
}

// Fonction qui renvoie le client de fd passé en argument
struct info_client*findClient_by_fd(struct info_client* clients,int fd){
	
  	if(clients==NULL){
		  
    	return NULL;
  	}
  	else{
		
    	struct info_client* tmp=clients;
    	while(tmp->next_client!=NULL){
			
      		if (tmp->fd == fd) {
				
        		return tmp;
			}                      
    		tmp=tmp->next_client;
			
        }
      	return tmp;
    }
}

struct info_client* AddNickname(struct info_client* clients, int fd,char nickname[]){
	if(clients==NULL)
		return NULL;
	if(clients->fd == fd){
		strcpy(clients->nickname, nickname);
		return clients;
	}
	else{
		clients->next_client=AddNickname(clients->next_client,fd,nickname);
		return clients;
	}
}

//Fonction qui renvoie 1 si le nickname est déjà attribué et zéro sinon
int verification_pseudo(struct info_client*clients,char nickname[]){
	int var = 0;
	if(clients == NULL)
		return 0;
	else{
		struct info_client* tmp=clients;
    	while(tmp->next_client!=NULL){
      		if (strcmp(tmp->nickname,nickname) == 0) {
        		var=1;
				goto end;
			}
			tmp=tmp->next_client;
		}
		if(strcmp(tmp->nickname,nickname) == 0){
			var=1;
			goto end;
		}
	}
	end:
	return var;
}

//Fonction qui renvoie 1 si le client de fd passé en argument est deja dans la liste 0 sinon (afin de gérer la réutilisation de /nick)
int verification_fd(struct info_client*clients,int sockfd){
	int var = 0;
	if(clients == NULL)
		return 0;
	else{
		struct info_client* tmp=clients;
    	while(tmp->next_client!=NULL){
      		if ((tmp->fd == sockfd)) {
        		var=1;
				goto end;
			}
			tmp=tmp->next_client;
		}
		if((tmp->fd == sockfd)){
			var=1;
			goto end;
		}
	}
	end:
	return var;
}

int change_pseudo(struct info_client*clients,int fd,char nickname[]){
	struct info_client* tmp=clients;
    while(tmp->next_client!=NULL){
		if((tmp->fd == fd)){
			
			strcpy(tmp->nickname,nickname);
			goto end;
		}
		tmp=tmp->next_client;
	}
	if((tmp->fd == fd)){
		
		strcpy(tmp->nickname,nickname);
		goto end;
	}	
	end:
	return 0;
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
	close(fd);
	return read_value;
}

void write_int_size(int fd, void *ptr) {
	int ret = 0, offset = 0;
	
	while (offset != sizeof(int)){
		ret = write(fd, ptr + offset, sizeof(int) - offset);
		if (-1 == ret){
			perror("Writing size");
			break;
		}
		offset += ret;
	}
	
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

void read_from_socket(int sockfd, char*buff, int size){
    int ret = 0;
	int offset = 0;
	
	while (offset != size) {
		ret = read(sockfd, buff+offset, size-offset);
		if (-1 == ret) {
			perror("Reading from client socket");
			exit(EXIT_FAILURE);
		}
		if (0 == ret) {
			printf("Should close connection, read 0 bytes\n");
			close(sockfd);
			//return -1;
			break;
		}
		offset += ret;
	}
}

//Fonction qui envoie au client le message d'authentification
void authentification(int sockfd){
	char*login = "[Server] : please login with /nick <your pseudo>";
	int size_login=strlen(login)+1;
	write_int_size(sockfd,(void*)&size_login);
	write_in_socket(sockfd,login,size_login);
}

//Fonction qui envoie un message de bienvenue au client nickname
void welcome_client(int sockfd,char*nickname){
	char buff[MSG_LEN];
	const char*msg="[Serveur] : Welcome on the chat ";
	strcpy(buff,msg);
	char*welcome = strcat(buff,nickname);
	int size=strlen(welcome);
	write_int_size(sockfd,&size);
	write_in_socket(sockfd,welcome,size);
}

//Fonction qui envoie au client les utilisateurs connectés
void who(int sockfd,char*str){
	char*msg="[SERVER] : online users are\n";
	char to_send[MSG_LEN];
	memset(to_send,0,MSG_LEN);
	strcpy(to_send,msg);
	strcat(to_send,str);
	int size = strlen(to_send);
	write_int_size(sockfd,&size);
	write_in_socket(sockfd,to_send,size);
}

//Fonction qui renvoie la date de connexion du client a été inspiré d'un exemple d'utilisation de la fonction time
char*time_client(char*buff){
	
	time_t connexion_time = time( NULL );
    struct tm * pTime = localtime( & connexion_time );
    strftime( buff, MSG_LEN, "%Y/%m/%d %H:%M", pTime );     
    return buff ;
}

//Fonction qui envoie au client les infos sur le client passé en argument
void whois(int sockfd,struct info_client*client){
	char to_send[MSG_LEN];
	memset(to_send,0,MSG_LEN);
	strcat(to_send,"[SERVER]:");
	strcat(to_send,client->nickname);
	strcat(to_send," is connected since ");
	strcat(to_send,client->date );
	strcat(to_send," with IP address ");
	strcat(to_send,client->addresse);
	strcat(to_send," and port number ");
	//convertir le port en chaine de caractères
	char port[10];
	if (sprintf(port, "%d", client->port) == -1) {
        perror("asprintf");
    } else {
        strcat(to_send, port);
    }
	int size = strlen(to_send);
	write_int_size(sockfd,&size);
	write_in_socket(sockfd,to_send,size);
}

void broadcast_message(struct info_client*clients,struct info_client *client_sender,char*buff){
	struct info_client*tmp=clients;
	char to_send[MSG_LEN];
	memset(to_send,0,MSG_LEN);
	strcpy(to_send,client_sender->nickname);
	strcat(to_send," says ");
	strcat(to_send,buff);
	int size_to_send = strlen(to_send)+1;
	printf("chaine a envoyer %s\n",to_send);
	afficherClient(tmp);
	
	printf("le fd du sender %d\n",client_sender->fd);
    while(tmp->next_client!=NULL){
		
		
			printf("mon fd est %d\n",tmp->fd);
			printf("dans if\n");
			afficherClient(tmp);
			write_int_size(tmp->fd,(void*)&size_to_send);
			write_in_socket(tmp->fd,to_send,size_to_send);
			printf("message ecrit dans %d : %s\n",tmp->fd,to_send);
    		tmp=tmp->next_client;
		
	}
	
	
		write_int_size(tmp->fd,(void*)&size_to_send);
		write_in_socket(tmp->fd,to_send,size_to_send);
	
	
	

}

void unicast_message(struct info_client*target_client,struct info_client*sender_client,int sockfd,char*buff,int size){
	
	char to_send[MSG_LEN];
	memset(to_send,0,MSG_LEN);
	strcat(to_send,"You received a message from ");
	strcat(to_send,sender_client->nickname);
	strcat(to_send," : ");
	strcat(to_send,buff);
	int size_to_send=size+strlen(to_send);
	write_int_size(sockfd,(void*)&size_to_send);
	write_in_socket(sockfd,to_send,size_to_send);
}

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
	struct info_client*liste_chainee = NULL;
	struct info_client*liste_clients = NULL;   
	struct info_client*Client_to_find;
	struct info_client*message_sender;
	char clients_connected[MSG_LEN];  
	memset(clients_connected,0,MSG_LEN);
	
	//Variables utilisées dans la liste chainée
	int fd_client=0;
	char address_client[MSG_LEN];
	memset(address_client,0,MSG_LEN);
	int client_port=0;
	char client_date[INFOS_LEN];
	memset(client_date,0,INFOS_LEN);
	char date[INFOS_LEN]={0};
	char to_send[MSG_LEN];
	memset(to_send,0,MSG_LEN);
	
    // Declare array of struct pollfd
	int nfds = 10;
	struct pollfd pollfds[nfds];

	// Init first slot with listening socket
	pollfds[0].fd = listen_fd;
	pollfds[0].events = POLLIN;
	pollfds[0].revents = 0;
	// Init remaining slot to default values
	int i;
	for (i = 1; i < nfds; i++) {
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
		int i;
		for (i = 0; i < nfds; i++) {
			
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
				
				//Ajoute les informations(sans nickname) du client à la liste chainée
				client_port=ntohs(sockptr->sin_port);
				
				strcpy(address_client,inet_ntoa(client_address));
				
				fd_client=client_fd;
				strcpy(client_date,time_client(date));
				
				
				liste_chainee=ajouterClient(liste_chainee,fd_client,client_port,client_date,address_client);
				afficherClient(liste_chainee);
				//demande d'authentification
				authentification(client_fd);
				
				// stocke le nouveau fd dans le tableau pollfds
				int j;
				for (j = 0; j < nfds; j++) {
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
			 if (pollfds[i].fd != listen_fd && pollfds[i].revents & POLLIN) {
				
				// //Read data from socket
				struct message msgstruct;
				memset(&msgstruct,0,sizeof(msgstruct));
				char buff[MSG_LEN];
				memset(buff,0,MSG_LEN);
				 
				
				
				
                read_from_socket(pollfds[i].fd,(void*)&msgstruct,sizeof(msgstruct));
				
				printf("le message reçue est de type %s\n",msg_type_str[msgstruct.type]);
				if (msgstruct.pld_len != 0){
					read_from_socket(pollfds[i].fd,buff,msgstruct.pld_len);
					
				}
				
				switch(msgstruct.type){
					case NICKNAME_NEW: ;
						
						if(verification_fd(liste_clients,pollfds[i].fd)==1){
							change_pseudo(liste_clients,pollfds[i].fd,msgstruct.infos);
							change_pseudo(liste_chainee,pollfds[i].fd,msgstruct.infos);
							goto end;
						}
						char*str="ERREUR : Ce pseudo est déjà attribué";
						int size_str = strlen(str)+1;
						if(verification_pseudo(liste_clients,msgstruct.infos)==1){
							write_int_size(pollfds[i].fd,(void*)&size_str);
							write_in_socket(pollfds[i].fd,str,size_str);
						}
						else{
						//Add the client's nickname to char client_connected
						
						strcat(clients_connected,"     -");
						strcat(clients_connected,msgstruct.infos);
						strcat(clients_connected,"\n");
							
						welcome_client(pollfds[i].fd,msgstruct.infos);
						
						//Add the element nickname
						
						liste_chainee=AddNickname(liste_chainee,pollfds[i].fd,msgstruct.infos);

						//Add the client with all his informations to liste_clients
						liste_clients=ajouterClient(liste_clients,fd_client,client_port,client_date,address_client);
						liste_clients=AddNickname(liste_clients,pollfds[i].fd,msgstruct.infos);
						
						}
						end:
						break;
						
					case NICKNAME_LIST:
						
						who(pollfds[i].fd,clients_connected);
						break;
					
					case NICKNAME_INFOS:
						
						afficherClient(liste_chainee);
						Client_to_find=findClient_by_pseudo(liste_chainee,msgstruct.infos);
						
						whois(pollfds[i].fd,Client_to_find);
						
						break;
					
					case BROADCAST_SEND:
						Client_to_find=findClient_by_fd(liste_chainee,pollfds[i].fd);
						broadcast_message(liste_chainee,Client_to_find,buff);
						break;

					case UNICAST_SEND:
						
						afficherClient(liste_chainee);
						Client_to_find=findClient_by_pseudo(liste_chainee,msgstruct.infos);  //correspond au client à qui on envoie le message
						
						message_sender=findClient_by_fd(liste_chainee,pollfds[i].fd);       //client qui envoie le message
						if(Client_to_find != NULL)

							unicast_message(Client_to_find,message_sender,Client_to_find->fd,buff,strlen(buff));
						else{
							strcpy(to_send,"user ");
							strcat(to_send,msgstruct.infos);
							strcat(to_send," does not exist");
							//char*msg="user does not exist";
							
							int size_msg=strlen(to_send)+1;
							write_int_size(pollfds[i].fd,(void*)&size_msg);
							write_in_socket(pollfds[i].fd,to_send,size_msg);
						}	
						break;
					case ECHO_SEND:

						write_int_size(pollfds[i].fd,(void*)&msgstruct.pld_len);
						write_in_socket(pollfds[i].fd,buff,msgstruct.pld_len);
						break;
					
					default:
						break;
						

				}
				pollfds[i].revents = 0;
				
			
			}
			
			
		}
	}
}
  

int main(int argc, char*argv[]) {
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


