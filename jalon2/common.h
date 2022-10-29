#define MSG_LEN 1024
#define SERV_PORT "8080"
#define SERV_ADDR "127.0.0.1"

struct info_client{
    int fd;
    int port;
    char date[128];
    char nickname[128];
    char*addresse;
   struct info_client*next_client;
};
