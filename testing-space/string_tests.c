#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>

#define IP_PORT "10800"

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

int main() {
    int i = 0; 
    while (1) {
        printf("%d. Warte 5 Sec\n", i++);
        sleep(5); 
        printf("5 Sekunden gewartet\n"); 
    }

    return 0;
}


/*
    HeaderManipulation modifications[] = {
        {"From,", "<sip:+49931333031@tel.t-online.de>;tag=asdfjkl3j1"},
        {"To", "<sip:+4915153710510@tel.t-online.de>"},
        {"Call-ID", "dkjlasdjoiujlkajdfkjh@192.168.178.54"},
        {"Contact", "<sip:+49199620000001598735@192.168.178.54:5060;transport=tcp>"}
    };

    char *name = modifications[0].header_name;

    if (strchr(modifications[0].header_name, ',') != NULL){
        printf(", gefunden"); 
    }
    name[strlen(name)-1] = '\0';
    strcpy(modifications[0].header_name, name);
    printf("%s\n", modifications[0].header_name);*/



//Analyse von Copieren, verschieben und ersetzen: 
/*
    char buffer[1024];

    snprintf(buffer, 1024,
    "Via: SIP/2.0/TCP 192.168.178.62:5060;branch=z9hG4bK776asdhds\r\n");

    char new_value[256];
    snprintf(new_value, 256, "SIP/2.0/TCP 192.168.178.62:5060;branch=z9hG4bK776eeeeasdhds");

//Start-Wert: 
    char *start = strstr(buffer, "Via"); 
    size_t len_start = strlen(start); 
    printf("Value von Start: %sLänge von Start: %ld\n\n", start, len_start);

//Ende der Zeile: 
    char *end = strstr(start, "\r\n");
    size_t len_end = strlen(end);
    printf("Value von End: %s%ld\n\n", end, len_end);  

//Value-Part ermitteln:
    char *value = strchr(start, ':'); 
    value +=2;
    printf("Value-Part: %s", value); 
    size_t len_value = strlen(value); 
    printf("Länge Value (+2 dazurechnen für ': '): %ld\n\n", len_value); 

//Value ersetzen: 
    strcat(new_value, "\r\n");
    size_t new_value_len = strlen(new_value); 
    printf("New Value: %s\n%ld\n\n", new_value, new_value_len);

    size_t len_before = strlen(value);
    printf("%ld\n", len_before);  
    memmove(value + new_value_len + 2, end, len_end);
    size_t len_after = strlen(value);
    printf("%ld\n", len_after);  
    printf("%s", value); 
    memcpy(value, new_value, new_value_len); 
    size_t len_new_value = strlen(value); 
    printf("%s%ld", value, len_new_value);

    char str[] = "Hallo Welt!"; 
    size_t hello_len1 = strlen(str); 
    printf("Vorher: %s\nLen: %ld\n", str, hello_len1); 
    memmove(str + 6, str, 5); 
    size_t hello_len2 = strlen(str); 
    printf("Nachher: %s\nLen: %ld\n", str, hello_len2); 

    //printf("---------------------------\n");
    size_t len = strlen(buffer); 
    //printf("%sLen: %ld\n", buffer, len);*/

/*
    char                ip_addr_ext[16] = "127.0.0.1";
    char                ip_port_ext[6]  = "10000";
    char                ip_port_int[6]  = "30800";
    struct sockaddr_in  sockaddr;

    size_t port_len = sizeof(ip_port_ext);
    printf("Len Port before: %ld\n", port_len);
    //load_config(ip_addr_ext, ip_port_ext, ip_port_int);
    //size_t port_len_af = sizeof(ip_port_ext); 
    //printf("Len Port after: %ld\n", port_len_af); 
    
    memset(&sockaddr, 0x00, sizeof(sockaddr));
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    sockaddr.sin_port = htons(atoi(IP_PORT));

    printf("%d\n", sockaddr.sin_port);

*/