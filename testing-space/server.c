//main.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <poll.h>
#include <time.h>
#include <signal.h>

int                 sockfd, connfd;
/*
#define IP_ADDRESS_EXT  "217.0.149.240"
#define IP_PORT_INT "5060"
#define IP_PORT_EXT "5060"
*/

void load_config(char *ip_address_ext, char *ip_port_ext, char *ip_port_int){
    FILE *config = fopen("./config.txt", "r"); 
    if (config == NULL) {
        printf("Unable to read config-file. Standard-values are used.\n"); 
        return;
    }

    char line[128];
    while (fgets(line, sizeof(line), config)) {
        if (strncmp(line, "IP_ADDRESS_EXT", 14) == 0) {
            sscanf(line, "IP_ADDRESS_EXT=%s", ip_address_ext);
        }else if (strncmp(line, "IP_PORT_INT", 11) == 0) {
            sscanf(line, "IP_PORT_INT=%s", ip_port_int);
        }else if (strncmp(line, "IP_PORT_EXT", 11) == 0) {
            sscanf(line, "IP_PORT_EXT=%s", ip_port_ext);
        }
    }
    fclose(config);
}

void handle_sigterm(int sig) {
    printf("SIGTERM empfangen, warte 30 Sekunden.\n"); 
    sleep(10);

    printf("Verbindung und Socket werden abgebaut}n");
    close(connfd); 
    close(sockfd); 

    printf("Server beenden\n"); 
    exit(0); 
}

int main(int argc, char* argv[])
{
    struct sockaddr_in  sockaddr, connaddr;
    unsigned int        connaddr_len;
    char                buffer[2048];
    int                 rv;



    //Default-Values (overwritten by config-file)
    char                ip_addr_ext[16] = "127.0.0.1";
    char                ip_port_ext[6]  = "10000";
    char                ip_port_int[6]  = "5060";

    printf("TCP server v1.3 (stand-alone-version) started...\n");
    printf("Loading config-file...\n");
    load_config(ip_addr_ext, ip_port_ext, ip_port_int); 
    printf("Following values are used:\nexternal IP: %s\texternal Port: %s\tinternal Port: '%s'\n\n", ip_addr_ext, ip_port_ext, ip_port_int);

    signal(SIGTERM, handle_sigterm); 
    //create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd == -1)
    {
        printf("ERROR: Socket creation failed ...\n\n");
        exit(EXIT_FAILURE);
    }
    else
        printf("Socket successfully created ...\n");

    //set IP address & IP port
    memset(&sockaddr, 0x00, sizeof(sockaddr));
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    sockaddr.sin_port = htons(atoi(ip_port_int));

    //bind socket
    if((bind(sockfd, (struct sockaddr*)&sockaddr, sizeof(sockaddr))) != 0)
    {
        printf("ERROR: Socket bind failed ...\n\n");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    else
        printf("Socket successfully binded ...\n");

    //listen to socket
    if((listen(sockfd, 5)) != 0)
    {
        printf("ERROR: Listen failed ...\n\n");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    else
        printf("Server listening ...\n\n");

    //server loop
    int timeout_count = 0; 
    while(timeout_count < 5) 
    {   
        printf("outter connfd: %d\n", connfd); 
        printf("outter sockfd: %d\n", sockfd); 
        connaddr_len = sizeof(connaddr);
        connfd = accept(sockfd, (struct sockaddr*)&connaddr, &connaddr_len);
        if(connfd < 0)
        {
            printf("error\n");
            close(sockfd); 
            exit(EXIT_FAILURE);
        }
        else
            printf("New connection accepted ...\n");


        //connection loop
        while(1)
        {
            printf("connfd: %d\n", connfd); 
            printf("sockfd: %d\n", sockfd); 
            //try to read from socket
            rv = read(connfd, buffer, sizeof(buffer));

            //close connection if error detected
            if(rv < 1)
            {
                printf("Connection closed ...\n\n");
                close(connfd);
                break;
            }
            else
                printf("%i bytes of data received...\n", rv);

            
            rv = write(connfd, buffer, sizeof(buffer));
            printf("Transmitted Buffer:\n\n %s\n", buffer); 

        }
    }
    close(sockfd); 

    return 0; 
}
