//main.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define IP_ADDRESS  "127.0.0.1"
#define IP_PORT     "10000"

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
    "Supported: 100rel,timer,histinfo\r\n"
    "User-Agent: Dennis-Test\r\n"
    "Allow: INVITE, ACK, CANCEL, OPTIONS, BYE\r\n"
    "Content-Length: 0\r\n"
    "\r\n",
    pcscf, pcscf, pcscf
    );
}


int main(int argc, char* argv[])
{
    int                 sockfd;
    struct sockaddr_in  sockaddr;
    char                buffer[1024];
    int                 rv;
    char                pcscf[256];


    printf("TCP client started ...\n");
    printf("Please enter the PCSCF-IP: ");
    fgets(pcscf, 256, stdin);

    size_t len = strlen(pcscf);
    if (len > 0 && pcscf[len -1] == '\n') {
        pcscf[len -1] = '\0';
    }

    create_sip_option(buffer, pcscf);
    printf("Following message will be send:\n%s\n", buffer);

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
        printf("Connection established ...\n\n");

    //send data to server
    rv = write(sockfd, buffer, strlen(buffer));
    printf("%i bytes of data transmitted...\n", rv);

    //receive data from server
    rv = read(sockfd, buffer, sizeof(buffer));
    printf("%i bytes of data received.\n\n", rv);
    printf("'\n%s'", buffer); 


    //disconnect and close socket
    printf("Terminate connection ...\n");
    shutdown(sockfd, SHUT_RDWR);
    close(sockfd);

    exit(EXIT_SUCCESS);
}
