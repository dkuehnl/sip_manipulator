//main.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define IP_ADDRESS  "192.168.178.54"
#define IP_PORT     "30080"

typedef struct {
    char header_name[64];
    char new_value[128];
} HeaderManipulation;


void create_sip_option(char *buffer, const char *pcscf) {
    snprintf(buffer, 1024,
    "OPTIONS sip:%s SIP/2.0\r\n"
    "Via: SIP/2.0/TCP 192.168.178.62:5060;branch=z9hG4bK776asdhds\r\n"
    "Max-Forwards: 70\r\n"
    "From: <sip:%s>;tag=djaiefkla348afikju3u9dkhjk3\r\n"
    "To: <sip:%s>\r\n"
    "Call-ID: jfköajbsödkivha@192.168.178.62\r\n"
    "CSeq: 102 OPTIONS\r\n"
    "Contact: <sip:+4919952000234234@192.168.178.62:5060;transport=tcp>\r\n"
    "User-Agent: Dennis-Test\r\n"
    "Allow: INVITE, ACK, CANCEL, OPTIONS, BYE\r\n"
    "Content-Length: 0\r\n"
    "\r\n",
    pcscf, pcscf, pcscf
    );
}

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

int main(int argc, char* argv[])
{
    int                 sockfd;
    struct sockaddr_in  sockaddr;
    char                buffer[1024];
    int                 rv;
    char                sip_message[1024];
    char                pcscf[256];

    HeaderManipulation modifications[] = {
        {"From", "<sip:+49931333031@tel.t-online.de>;tag=asdfjkl3j1"},
        {"To", "<sip:+4915153710510@tel.t-online.de>"},
        {"Call-ID", "dkjlasdjoiujlkajdfkjh@192.168.178.54"},
        {"Contact", "<sip:+49199620000001598735@192.168.178.54:5060;transport=tcp>"}
    };

    int num_modifications = sizeof(modifications) / sizeof(modifications[0]);

    printf("TCP client started ...\n");
    printf("Please enter the PCSCF-IP: ");
    fgets(pcscf, 64, stdin);

    size_t len = strlen(pcscf);
    if (len > 0 && pcscf[len -1] == '\n') {
        pcscf[len -1] = '\0';
    }

    create_sip_option(sip_message, pcscf);
    printf("Following message will be send: %s\n", sip_message);

    //create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd == -1)
    {
        printf("ERROR: Socket creation failed ...\n\n");
        exit(EXIT_FAILURE);
    }
    else
        printf("Socket successfully created ...\n");

    //set destination IP address & IP port (server)
    memset(&sockaddr, 0x00, sizeof(sockaddr));
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_addr.s_addr = inet_addr(IP_ADDRESS);
    sockaddr.sin_port = htons(atoi(IP_PORT));

    //try to connect client to server
    if(connect(sockfd, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) != 0)
    {
        printf("ERROR: Connect failed ...\n\n");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    else
        printf("Connection established ...\n");

    //send data to server
    strcpy(buffer, sip_message);
    rv = write(sockfd, buffer, strlen(buffer));
    printf("%i bytes of data transmitted...\n", rv);

    //receive data from server
    rv = read(sockfd, buffer, sizeof(buffer));
    printf("%i bytes of data received...\nWill be processed now...\n", rv);

    char mod_flag[2];
    printf("Want to search in reveived data? (y/n): ");
    scanf("%s", &mod_flag);
    if (strncmp(mod_flag, "y", 1) == 0) {
        process_header(buffer, modifications, num_modifications);

        printf("\n----------------------------------\n");
        printf("Mirroring Package against server...\n");

        rv = write(sockfd, buffer, strlen(buffer));
        printf("%i bytes of data transmitted.\n", rv);
        rv = read(sockfd, buffer, sizeof(buffer));
        printf("%i bytes of data received.\n\n", rv);
        printf("%s\n", buffer);
    }

    //disconnect and close socket
    printf("Terminate connection ...\n");
    shutdown(sockfd, SHUT_RDWR);
    close(sockfd);

    exit(EXIT_SUCCESS);
}
