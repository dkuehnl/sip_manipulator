#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <arpa/inet.h>

char test[64];

void signal_handler(int signum) {
    if (signum == SIGHUP) {
        printf("Mirror == 0, %s \n", test); 
    }
}

void signal_handler2(int signum) {
    if (signum == SIGHUP) {
        printf("Mirror == 1 \n"); 
    }
}

int main() {
    struct sigaction *sa;
    int mirror = 0; 

    strcpy(test, "Test123");

    if (mirror == 0) {
        sa->sa_handler = signal_handler;
        sa.sa_flags = 0; 
        sigemptyset(&sa.sa_mask);
        if (sigaction(SIGHUP, &sa, NULL) == -1){
            printf("Fehler\n");
            exit(EXIT_FAILURE); 
        }
    } else if (mirror == 1) {
        sa.sa_handler = signal_handler2;
        sa.sa_flags = 0; 
        sigemptyset(&sa.sa_mask);
        if (sigaction(SIGHUP, &sa, NULL) == -1){
            printf("Fehler\n");
            exit(EXIT_FAILURE); 
        }
    }
    

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(8080);

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("bind");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    listen(sockfd, 5);
    
    printf("Server is running. Send SIGHUP to reload the file.\n");

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int connfd = accept(sockfd, (struct sockaddr *)&client_addr, &client_len);
        if (connfd < 0) {
            perror("accept");
            continue;
        }

        // Bearbeite eingehende Verbindung (hier Platz für deine Logik)
        printf("New TCP connection accepted\n");
        close(connfd);
    }

    close(sockfd);
    return 0;
}