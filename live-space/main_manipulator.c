//manipulator-function 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <sys/time.h>

#include "log.h"

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
    FILE *file = fopen("/usr/src/app/hmr.txt", "w"); 
    for (int i = 0; i < num_header_manipulations; i++) {
        fprintf(file, "%s, %s\n", modifications[i].header_name, modifications[i].new_value);
    }
    fclose(file);
}

HeaderManipulation* load_hmr_from_file(int *loaded_entries, FILE* logfile) {
    FILE *file = fopen("/usr/src/app/hmr.txt", "r"); 
    if (file == NULL) {
        fprintf(logfile, "%s - ERR LOAD_HMR: File couldn't be opened!\n", get_timestamp()); 
        return NULL;
    }

    int capacity = 10;
    int count = 0; 

    HeaderManipulation *modifications = malloc(capacity * sizeof(HeaderManipulation)); 
    if (modifications == NULL) {
        fprintf(logfile, "%s - ERR LOAD_HMR: Couldn't reserve memory\n", get_timestamp()); 
        fclose(file); 
        return NULL;
    }

    while (fscanf(file, "%63s %127s", modifications[count].header_name, modifications[count].new_value) == 2){
        count++;

        if (count >= capacity) {
            capacity *= 2;
            HeaderManipulation *temp = realloc(modifications, capacity * sizeof(HeaderManipulation));
            if (temp == NULL) {
                fprintf(logfile, "%s - ERR LOAD_HMR: Realloc failed\n", get_timestamp());
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

void load_config(char *ip_address_ext, char *ip_port_ext, char *ip_port_int, FILE* logfile){
    FILE *config = fopen("/usr/src/app/config.txt", "r"); 
    if (config == NULL) {
        fprintf(logfile, "%s - ERR LOAD_CONFIG: Unable to read config-file. Standard-values are used.\n", get_timestamp()); 
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

int main()
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


    //Start Logging
    FILE *logfile_server = fopen("/usr/src/app/log_manipulator.txt", "a"); 
    fprintf(logfile_server, "%s - SIP-Manipulator v2.0 startet...\n", get_timestamp());
    fprintf(logfile_server, "%s - Loading config-file...\n", get_timestamp());
    load_config(ip_addr_ext, ip_port_ext, ip_port_int, logfile_server); 
    fprintf(logfile_server, "%s - Following values are used:\nexternal IP: %s\texternal Port: %s\tinternal Port: '%s'\n\n", get_timestamp(), ip_addr_ext, ip_port_ext, ip_port_int);

    //create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd == -1)
    {
        fprintf(logfile_server, "%s - ERROR MAIN: Socket creation failed ...\n\n", get_timestamp());
        fclose(logfile_server);
        exit(EXIT_FAILURE);
    }

    //set IP address & IP port
    memset(&sockaddr, 0x00, sizeof(sockaddr));
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    sockaddr.sin_port = htons(atoi(ip_port_int));

    //bind socket
    if((bind(sockfd, (struct sockaddr*)&sockaddr, sizeof(sockaddr))) != 0)
    {
        fprintf(logfile_server,"%s - ERROR MAIN: Socket bind failed ...\n\n", get_timestamp());
        close(sockfd);
        fclose(logfile_server);
        exit(EXIT_FAILURE);
    }

    //listen to socket
    if((listen(sockfd, 5)) != 0)
    {
        fprintf(logfile_server, "%s - ERROR MAIN: Listen failed ...\n\n", get_timestamp());
        close(sockfd);
        fclose(logfile_server);
        exit(EXIT_FAILURE);
    }
    else
        fprintf(logfile_server, "%s - Server listening ...\n\n", get_timestamp());

    //Load HMR's
    HeaderManipulation *modifications = load_hmr_from_file(&load_count, logfile_server);
    if (modifications == NULL) {
        fprintf(logfile_server, "%s - ERROR MAIN: HMR could not be loaded.\n\n", get_timestamp()); 
        //exit(EXIT_FAILURE);
    }

    //Define Timeout: 
    timeout.tv_sec = 30;
    timeout.tv_usec = 0;
    if(setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0){
        fprintf(logfile_server, "%s - ERROR MAIN: Problem with timeout-creation\n", get_timestamp()); 
        close(sockfd); 
        exit(EXIT_FAILURE); 
    }

    //server loop
    while(1)
    {
        fprintf(logfile_server, "%s - Waiting for incoming connection...\n", get_timestamp()); 
        connaddr_len = sizeof(connaddr);
        connfd = accept(sockfd, (struct sockaddr*)&connaddr, &connaddr_len);
        if(connfd < 0)
        {
            if(errno == EAGAIN || errno == EWOULDBLOCK) {
                //printf("Timeout connection\n");
                continue;
            } else {
                fprintf(logfile_server, "%s - ERROR SER_LOOP: Something went wrong with accepting the connection \n", get_timestamp()); 
                //close(sockfd); 
                continue;
            }
        }
        else
            fprintf(logfile_server, "%s - New connection accepted ...\n", get_timestamp());

        //2nd-Connection
        sockfd_ext = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd_ext == -1) {
            fprintf(logfile_server, "%s - ERR 2ND-CON: Creating of externel Socket failed...\n", get_timestamp());
            break;
        }

        memset(&sockaddr_ext, 0x00, sizeof(sockaddr_ext));
        sockaddr_ext.sin_family = AF_INET;
        sockaddr_ext.sin_addr.s_addr = inet_addr(ip_addr_ext);
        sockaddr_ext.sin_port = htons(atoi(ip_port_ext));

        if(connect(sockfd_ext, (struct sockaddr*)&sockaddr_ext, sizeof(sockaddr_ext)) != 0) {
            fprintf(logfile_server, "%s - ERROR 2ND-CON: Connect to external server failed..\n", get_timestamp());
            close(sockfd_ext); 
            exit(EXIT_FAILURE);
        } else {
            fprintf(logfile_server, "%s - Connection to %s established successfully. \n", get_timestamp(), ip_addr_ext);
        }

        //connection loop
        while(1)
        {
            //try to read from socket
            rv = read(connfd, buffer, sizeof(buffer));

            //close connection if error detected
            if(rv < 1)
            {
                fprintf(logfile_server, "%s - Connection closed ...\n\n", get_timestamp());
                close(connfd);
                close(sockfd_ext);
                break;
            }
            else
                fprintf(logfile_server, "%s - %i bytes of data received...\n", get_timestamp(), rv);

            if (load_count > 0)
            {
                process_header(buffer, modifications, load_count);
            }

            rv_ext = write(sockfd_ext, buffer, strlen(buffer));
            fprintf(logfile_server, "%s - Transmitted Buffer:\n\n %s\n", get_timestamp(), buffer);
            memset(buffer, 0, sizeof(buffer));
            rv_ext = read(sockfd_ext, buffer, sizeof(buffer));
            fprintf(logfile_server, "%s - %i bytes of data received from external server\n", get_timestamp(), rv_ext);

            rv = write(connfd, buffer, sizeof(buffer));
            fprintf(logfile_server, "%s - Transmitted Buffer:\n\n %s\n", get_timestamp(), buffer); 
        }
    }

    fprintf(logfile_server, "%s - Stop-Signal from main-application detected\n", get_timestamp());
    close(sockfd); 
    fclose(logfile_server);  

    return 0;
}
