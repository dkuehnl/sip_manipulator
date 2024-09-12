//main.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define IP_ADDRESS_OWN  "192.168.178.54"
#define IP_PORT_INT "5060"
#define IP_PORT_EXT "30080"

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
    FILE *file = fopen("/usr/local/bin/hmr.txt", "w"); 
    for (int i = 0; i < num_header_manipulations; i++) {
        fprintf(file, "%s, %s\n", modifications[i].header_name, modifications[i].new_value);
    }
    fclose(file);
}

HeaderManipulation* load_hmr_from_file(int *loaded_entries) {
    FILE *file = fopen("/usr/local/bin/hmr.txt", "r"); 
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

int main(int argc, char* argv[])
{
    int                 sockfd, connfd;
    struct sockaddr_in  sockaddr, connaddr;
    unsigned int        connaddr_len;
    char                buffer[2048];
    int                 rv;
    int                 load_count = 0;


    printf("TCP server started ...\n");

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
    sockaddr.sin_port = htons(atoi(IP_PORT_INT));

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

    //server loop
    while(1)
    {
        connaddr_len = sizeof(connaddr);
        connfd = accept(sockfd, (struct sockaddr*)&connaddr, &connaddr_len);
        if(connfd < 0)
        {
            printf("ERROR: Server accept failed ...\n\n");
            break;
        }
        else
            printf("New connection accepted ...\n");

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
                break;
            }
            else
                printf("%i bytes of data received...\n", rv);

            if (load_count > 0)
            {
                process_header(buffer, modifications, load_count);
            }

            rv = write(connfd, buffer, strlen(buffer) + 1);
            printf("Transmitted Buffer:\n\n %s\n", buffer);
        }
    }
}
