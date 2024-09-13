#include <stdio.h>
#include <string.h>

typedef struct {
    char header_name[64];
    char new_value[128];
} HeaderManipulation;

int main() {
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

/*    char str[] = "Hallo Welt!"; 
    size_t hello_len1 = strlen(str); 
    printf("Vorher: %s\nLen: %ld\n", str, hello_len1); 
    memmove(str + 6, str, 5); 
    size_t hello_len2 = strlen(str); 
    printf("Nachher: %s\nLen: %ld\n", str, hello_len2); 
*/
    //printf("---------------------------\n");
    size_t len = strlen(buffer); 
    //printf("%sLen: %ld\n", buffer, len);

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