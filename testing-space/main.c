#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef struct {
    char header_name[64];
    char new_value[128];
} HeaderManipulation;

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

void save_hmr(HeaderManipulation *modifications, int num_header_manipulations) {
    FILE *file = fopen("hmr.txt", "w"); 
    for (int i = 0; i < num_header_manipulations; i++) {
        fprintf(file, "%s, %s\n", modifications[i].header_name, modifications[i].new_value);
    }
    fclose(file);
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

int main() {
    char    buffer[1024];
    int     load_count = 0;
    // char    header[128];

    snprintf(buffer, 1024,
    "OPTIONS sip:tel SIP/2.0\r\n"
    "Via: SIP/2.0/TCP 192.168.178.62:5060;branch=z9hG4bK776asdhds\r\n"
    "Max-Forwards: 70\r\n"
    "From: <sip:tel>;tag=djaiefkla348afikju3u9dkhjk3\r\n"
    "To: <sip:tel>\r\n"
    "Call-ID: jfköajbsödkivha@192.168.178.62\r\n"
    "CSeq: 102 OPTIONS\r\n"
    "Contact: <sip:+4919952000234234@192.168.178.62:5060;transport=tcp>\r\n"
    "User-Agent: Dennis-Test\r\n"
    "Allow: INVITE, ACK, CANCEL, OPTIONS, BYE\r\n"
    "Content-Length: 0\r\n"
    "\r\n");

    /*HeaderManipulation modifications[] = {
        {"From", "<sip:+49931333031@tel.t-online.de>;tag=asdfjkl3j1"},
        {"To", "<sip:+4915153710510@tel.t-online.de>"},
        {"Call-ID", "dkjlasdjoiujlkajdfkjh@192.168.178.54"},
        {"Contact", "<sip:+49199620000001598735@192.168.178.54:5060;transport=tcp>"}
    };*/
    
    HeaderManipulation *modifications = load_hmr_from_file(&load_count);

    printf("Following modifications are saved: \n"); 
    for (int i = 0; i < load_count; i++) {
        printf("%d: %s %s\n", i+1, modifications[i].header_name, modifications[i].new_value);
    }
    printf("\n\n");
    process_header(buffer, modifications, load_count);
    printf("\n%s\n", buffer);
    //save_hmr(modifications, num_modifications); 
    return 0;
}
