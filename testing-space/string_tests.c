#include <stdio.h>
#include <string.h>

typedef struct {
    char header_name[64];
    char new_value[128];
} HeaderManipulation;

int main() {
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
    printf("%s\n", modifications[0].header_name);
    return 0;
}