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

/*
#define IP_ADDRESS_EXT  "217.0.149.240"
#define IP_PORT_INT "5060"
#define IP_PORT_EXT "5060"
*/
typedef struct {
    char header_name[64];
    char new_value[128];
} HeaderManipulation;

void hmr(char *sip_message, const char *header_name, char *new_value) {
    char *header_start = strstr(sip_message, header_name);

    if (header_start) {
        char *line_end = strstr(header_start, "\r\n");

        if (line_end) {
            char *value_start = strchr(header_start, ':');

            if (value_start && value_start < line_end) {
                value_start++;
                while(*value_start == ' ') value_start++;
                size_t new_value_length = strlen(new_value);
                size_t remaining_message_length = strlen(line_end);

                memmove(value_start + new_value_length, line_end, remaining_message_length + 1);
                memcpy(value_start, new_value, new_value_length);
                // printf("%s", value_start);
            }
        }
    }

}

void process_header(char *sip_message, HeaderManipulation *modifications, int num_modifications) {
    for (int i = 0; i < num_modifications; i++) {
        hmr(sip_message, modifications[i].header_name, modifications[i].new_value);
    }
}

void save_hmr(HeaderManipulation *modifications, int num_header_manipulations) {
    FILE *file = fopen("./hmr.txt", "w"); 
    for (int i = 0; i < num_header_manipulations; i++) {
        fprintf(file, "%s, %s\n", modifications[i].header_name, modifications[i].new_value);
    }
    fclose(file);
}

HeaderManipulation* load_hmr_from_file(int *loaded_entries) {
    FILE *file = fopen("./hmr.txt", "r"); 
    if (file == NULL) {
        printf("Err: File couldn't be opened!\n"); 
        return NULL;
    }

    int capacity = 10;
    int count = 0; 

    HeaderManipulation *modifications = malloc(capacity * sizeof(HeaderManipulation)); 
    if (modifications == NULL) {
        printf("Err: Couldn't reserve memory\n"); 
        fclose(file); 
        return NULL;
    }

    while (fscanf(file, "%63s %127s", modifications[count].header_name, modifications[count].new_value) == 2){
        count++;

        if (count >= capacity) {
            capacity *= 2;
            HeaderManipulation *temp = realloc(modifications, capacity * sizeof(HeaderManipulation));
            if (temp == NULL) {
                printf("Err: Realloc failed\n");
                free(modifications); 
                fclose(file); 
                return NULL;
            }
            modifications = temp;
        }
    }

    fclose(file);
    *loaded_entries = count;

    for (int i = 0; i < count; i++) {
        if (strchr(modifications[i].header_name, ',') != NULL){
            char *name = modifications[i].header_name; 
            name[strlen(name)-1] = '\0';
            strcpy(modifications[i].header_name, name); 
        }
    }
    return modifications;
}

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

int main(int argc, char* argv[])
{
    int                 sockfd, connfd, sockfd_ext;
    struct sockaddr_in  sockaddr, connaddr, sockaddr_ext;
    unsigned int        connaddr_len;
    char                buffer[2048];
    int                 rv, rv_ext;
    int                 load_count = 0;
    struct timeval      timeout;



    //Default-Values (overwritten by config-file)
    char                ip_addr_ext[16] = "127.0.0.1";
    char                ip_port_ext[6]  = "10000";
    char                ip_port_int[6]  = "5060";

    printf("TCP server v1.3 (stand-alone-version) started...\n");
    printf("Loading config-file...\n");
    load_config(ip_addr_ext, ip_port_ext, ip_port_int); 
    printf("Following values are used:\nexternal IP: %s\texternal Port: %s\tinternal Port: '%s'\n\n", ip_addr_ext, ip_port_ext, ip_port_int);

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

    printf("%d\n", sockaddr.sin_port);
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

    //Load HMR's
    HeaderManipulation *modifications = load_hmr_from_file(&load_count);
    if (modifications == NULL) {
        printf("ERROR: HMR could not be loaded.\n\n"); 
        //exit(EXIT_FAILURE);
    } else {
        printf("HMR successfully loaded..\nReady to proceed...\n");
    }

    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    if(setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0){
        printf("Err setsockopt\n"); 
        close(sockfd); 
        exit(EXIT_FAILURE); 
    }

    //server loop
    int timeout_count = 0; 
    while(timeout_count < 5) 
    {   
        printf("%d. loop\n\n", timeout_count);
        timeout_count++;
        connaddr_len = sizeof(connaddr);
        connfd = accept(sockfd, (struct sockaddr*)&connaddr, &connaddr_len);
        if(connfd < 0)
        {
            if(errno == EAGAIN || errno == EWOULDBLOCK) {
                printf("Timeout connection\n");
                continue;
            } else {
                printf("other accept-error\n"); 
                close(sockfd); 
                exit(EXIT_FAILURE);
            }
        }
        else
            printf("New connection accepted ...\n");

        //2nd-Connection
        sockfd_ext = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd_ext == -1) {
            printf("Creating of externel Socket failed...\n");
            break;
        } else {
            printf("Socket for external connection created.\n"); 
        }

        memset(&sockaddr_ext, 0x00, sizeof(sockaddr_ext));
        sockaddr_ext.sin_family = AF_INET;
        sockaddr_ext.sin_addr.s_addr = inet_addr(ip_addr_ext);
        sockaddr_ext.sin_port = htons(atoi(ip_port_ext));

        if(connect(sockfd_ext, (struct sockaddr*)&sockaddr_ext, sizeof(sockaddr_ext)) != 0) {
            printf("ERROR: Connect to external server failed..\n");
            close(sockfd_ext); 
            exit(EXIT_FAILURE);
        } else {
            printf("Connection so %s established successfully. \n", ip_addr_ext);
        }

        //connection loop
        while(1)
        {
            //try to read from socket
            rv = read(connfd, buffer, sizeof(buffer));

            //close connection if error detected
            if(rv < 1)
            {
                printf("Connection closed ...\n\n");
                close(connfd);
                close(sockfd_ext);
                break;
            }
            else
                printf("%i bytes of data received...\n", rv);

            if (load_count > 0)
            {
                process_header(buffer, modifications, load_count);
            }

            rv_ext = write(sockfd_ext, buffer, strlen(buffer));
            printf("Transmitted Buffer:\n\n %s\n", buffer);
            memset(buffer, 0, sizeof(buffer));
            rv_ext = read(sockfd_ext, buffer, sizeof(buffer));
            printf("%i bytes of data received from external server\n", rv_ext);

            rv = write(connfd, buffer, sizeof(buffer));
            printf("Transmitted Buffer:\n\n %s\n", buffer); 

        }
    }
    close(sockfd); 

    return 0; 
}
