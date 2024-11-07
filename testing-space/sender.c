//main.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define IP_ADDRESS  "127.0.0.1"
#define IP_PORT     "5060"

void create_sip_option(char *buffer, const char *pcscf, const char* sip) {
    snprintf(buffer, 2048,
    "INVITE sip:+49211522891124@87.138.26.105:5060;transport=TCP SIP/2.0\r\n"
    "Via: SIP/2.0/TCP 217.0.144.89:5060;branch=z9hG4bK776asdhdsdafdsf674\r\n"
    "Max-Forwards: 68\r\n"
    "From: <sip:+4915153710510@t-mobile.de;user=phone>;tag=djaiefkla348afikju3u9dkhjk3\r\n"
    "To: <sip:+49211522891124@tel.t-online.de;user=phone>\r\n"
    "Call-ID: jfkdajbs32dkivha@192.168.178.62\r\n"
    "CSeq: 1082 INVITE\r\n"
    "Contact: <sip:+4919952000234234@192.168.178.62:5060;transport=tcp>\r\n"
    "Record-Route: <sip:217.0.144.89:5060;transport=tcp;lr>\r\n"
    "Accept-Contact: *;explicit;description=\"<sip:+49199296000000012450@tel.t-online.de>\";require\r\n"
    "MIN_SE: 900\r\n"
    "P-Asserted-Identity: <tel:+49211522891291>\r\n"
    "P-Asserted-Identity: <sip:+49211522891291@tel.t-online.de;user=phone>\r\n"
    "P-Early-Media: supported\r\n"
    "Privacy: none\r\n"
    "Session Expires: 1800;refresher=uac\r\n"
    "x-Ericsson-SIP-Source: 217.0.144.89:5060\r\n"
    "x-Ericsson-SIP-Destination: 89.138.26.105:55320\r\n"
    "Supported: 100rel,timer,histinfo\r\n"
    "User-Agent: Dennis-Test\r\n"
    "Allow: INVITE, ACK, CANCEL, OPTIONS, BYE\r\n"
    );
}


int main(int argc, char* argv[])
{
    int                 sockfd;
    struct sockaddr_in  sockaddr;
    char                buffer[2048];
    int                 rv;
    char                sdp[1024];

    snprintf(sdp, 1024, 
    "Content-Length: 326\r\n"
    "Content-Type: application/sdp\r\n"
    "\r\n"
    "v=0\r\n"
    "o=alice 2890844526 2890842807 IN IP4 pc33.atlanta.com\r\n"
    "s=Session SDP\r\n"
    "c=IN IP4 192.0.2.1\r\n"
    "t=0 0\r\n"
    "m=audio 49170 RTP/AVP 0 8 118 119\r\n"
    "a=rtpmap:0 PCMU/8000\r\n"
    "a=rtpmap:8 PCMA/8000\r\n"
    "a=rtpmap:118 telephone-event/8000\r\n"
    "a=fmtp: 0-15\r\n"
    "a=rtpmap:119 telephone-event/16000\r\n"
    "a=fmtp:0 annexb=no\r\n"
    "a=ptime:20\r\n"
    "a=maxptime:150\r\n"
    "a=sendrecv\r\n"
    );

    printf("TCP client started ...\n");
    /*printf("Please enter the PCSCF-IP: ");
    fgets(pcscf, 256, stdin);

    size_t len = strlen(pcscf);
    if (len > 0 && pcscf[len -1] == '\n') {
        pcscf[len -1] = '\0';
    }*/

    create_sip_option(buffer, "1.1.1.1", sdp);
    printf("Following message will be send:\n%ld\n%s", strlen(buffer), buffer);

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
    printf("1. %i bytes of data transmitted...\n", rv);
    sleep(10);
    rv = write(sockfd, sdp, strlen(sdp));
    printf("2. %i bytes of data transmitted...\n", rv);

    //receive data from server
    rv = read(sockfd, buffer, sizeof(buffer));
    printf("%i bytes of data received.\n\n", rv);
    printf("'\n%s'", buffer); 


    //disconnect and close socket
    sleep(30);
    printf("Terminate connection ...\n");
    shutdown(sockfd, SHUT_RDWR);
    close(sockfd);

    exit(EXIT_SUCCESS);
}
