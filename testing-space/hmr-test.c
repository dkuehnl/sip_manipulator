#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <signal.h>
#include <time.h>
#include <osip2/osip.h>
#include <osipparser2/osip_const.h>
#include <osipparser2/osip_headers.h>
#include <osipparser2/osip_body.h>
#include <osipparser2/osip_message.h>

#define IP_PORT "10800"

typedef struct {
    char *message_type;
    char **headers;
    char **new_values;
    int count;
} ManipulationEntry;

typedef struct {
    ManipulationEntry *entries;
    int count;
} ManipulationTable;

ManipulationTable *load_hmr(const char *hmr_path){ 
    ManipulationTable *table = malloc(sizeof(ManipulationTable));
    table->entries = NULL;
    table->count = 0;

    FILE *hmr = fopen(hmr_path, "r"); 
    if (hmr == NULL) {
        printf("Error while reading file\n"); 
        return table;
    }
    char line[256];
    ManipulationEntry currentEntry = {0};
    while(fgets(line, sizeof(line), hmr)){
        if (line[0] == '\n'){
            continue;
        }
        line[strcspn(line, "\n")] = 0; 
        if (line[0] == '#') {
            if(currentEntry.message_type) {
                table->entries = realloc(table->entries, sizeof(ManipulationEntry) * (table->count + 1));
                table->entries[table->count++] = currentEntry;
                currentEntry = (ManipulationEntry){0};
            }
            currentEntry.message_type = strdup(line + 1);
            currentEntry.headers = NULL;
            currentEntry.new_values = NULL;
            currentEntry.count = 0;
        } else if (currentEntry.message_type) {
            char *header = strtok(line, ","); 
            char *new_value = strtok(NULL, ","); 

            currentEntry.headers = realloc(currentEntry.headers, sizeof(char*) * (currentEntry.count + 1));
            currentEntry.new_values = realloc(currentEntry.new_values, sizeof(char*) * (currentEntry.count +1));
            currentEntry.headers[currentEntry.count] = strdup(header); 
            currentEntry.new_values[currentEntry.count] = strdup(new_value); 
            currentEntry.count++;
        }
       
    }
    
    if (currentEntry.message_type) {
        table->entries = realloc(table->entries, sizeof(ManipulationEntry) * (table->count +1));
        table->entries[table->count++] = currentEntry;
    }

    fclose(hmr); 
    return table;
}


void create_sip_option(char *buffer, const char *pcscf) {
    snprintf(buffer, 1024,
    "OPTIONS sip:test@tel.t-online.de SIP/2.0\r\n"
    "Via: SIP/2.0/TCP 192.168.178.62:5060;branch=z9hG4bK776asdhdsdafdsf674\r\n"
    "Via: SIP/2.0/TCP 192.168.178.1:5060;branch=z9hG4bK776asdhdsdafdsf674\r\n"
    "Via: SIP/2.0/TCP 8.8.8.8:5060;branch=z9hG4bK776asdhdsdafdsf674\r\n"
    "Max-Forwards: 70\r\n"
    "From: <sip:+49789554324564@192.168.178.5>;tag=djaiefkla348afikju3u9dkhjk3\r\n"
    "To: <sip:%s>\r\n"
    "X-Erricson: Test\r\n"
    "P-Asserted-Identity: <sip:+49123456@tel.t-online.de;user=phone>\r\n"
    "Call-ID: jfkdajbs32dkivha@192.168.178.62\r\n"
    "CSeq: 1082 OPTIONS\r\n"
    "Contact: <sip:+4919952000234234@192.168.178.62:5060;transport=tcp>\r\n"
    "Supported: 100rel,timer,histinfo\r\n"
    "User-Agent: Dennis-Test\r\n"
    "Allow: INVITE, ACK, CANCEL, OPTIONS, BYE\r\n"
    "Content-Length: 0\r\n\r\n",
    pcscf
    );
}

void manipulate_existing_header(char *buffer, ManipulationEntry entries) {
    osip_t *osip;
    if (osip_init(&osip)!=0) {
        return;
    }
    
    osip_message_t *sip; 
    osip_message_init(&sip);
    if (osip_message_parse(sip, buffer, strlen(buffer))!=0){
        printf("Error while parsing\n");
        return;
    }

    for (int i = 0; i < entries.count; i++){
        if (strncmp(entries.headers[i], "From", 4) == 0) {
            if (strncmp(entries.new_values[i], " host", 5) == 0){
                sip->from->url->host = strchr(entries.new_values[i], ':')+1;
            } else if (strncmp(entries.new_values[i], " user", 5) == 0) {
                sip->from->url->username = strchr(entries.new_values[i], ':')+1;
            } else if (strncmp(entries.new_values[i], " scheme", 7) == 0){
                sip->from->url->scheme = strchr(entries.new_values[i], ':')+1;
            }
        } else if (strncmp(entries.headers[i], "To", 2) == 0) {
            if (strncmp(entries.new_values[i], " host", 5) == 0){
                sip->to->url->host = strchr(entries.new_values[i], ':')+1;
            } else if (strncmp(entries.new_values[i], " user", 5) == 0) {
                sip->to->url->username = strchr(entries.new_values[i], ':')+1;
            } else if (strncmp(entries.new_values[i], " scheme", 7) == 0){
                sip->to->url->scheme = strchr(entries.new_values[i], ':')+1;
            }
        } else if (strncmp(entries.headers[i], "Call-ID", 7) == 0) {
            if (strncmp(entries.new_values[i], " host", 5) == 0){
                sip->call_id->host = strchr(entries.new_values[i], ':')+1;
            } else if (strncmp(entries.new_values[i], " number", 7) == 0) {
                sip->call_id->number = strchr(entries.new_values[i], ':')+1;
            }
        } else if (strncmp(entries.headers[i], "Contact", 7) == 0) {
            osip_contact_t *contact;
            osip_message_get_contact(sip, 0, &contact);
            if (strncmp(entries.new_values[i], " scheme", 7) == 0){
                contact->url->scheme = strchr(entries.new_values[i], ':')+1;
            } else if (strncmp(entries.new_values[i], " user", 5) == 0) {
                contact->url->username = strchr(entries.new_values[i], ':')+1;
            } else if (strncmp(entries.new_values[i], " host", 5) == 0) {
                contact->url->host = strchr(entries.new_values[i], ':')+1;
            } else if (strncmp(entries.new_values[i], " port", 5) == 0) {
                contact->url->port = strchr(entries.new_values[i], ':')+1;
            }
        } else if (strncmp(entries.headers[i], "CSeq", 7) == 0) {
            if (strncmp(entries.new_values[i], " number", 7) == 0){
                sip->cseq->number = strchr(entries.new_values[i], ':')+1;
            } else if (strncmp(entries.new_values[i], " method", 7) == 0) {
                sip->cseq->method = strchr(entries.new_values[i], ':')+1;
            }
        } else if (strncmp(entries.headers[i], "Via", 7) == 0) {
            continue;
        } else {
            osip_header_t *header;
            if (osip_message_header_get_byname(sip, entries.headers[i], 0, &header) >  0){
                if (strncmp(entries.new_values[i], " del", 4) == 0) {
                    osip_header_t *curr_header;
                    int pos = 0;

                    while (!osip_list_eol(&sip->headers, pos)) {
                        curr_header = (osip_header_t *)osip_list_get(&sip->headers, pos);
                        if (curr_header == header) {
                            osip_list_remove(&sip->headers, pos);
                            break;
                        }
                        pos++;
                    }
                } else {
                    header->hvalue = entries.new_values[i];
                }
            } else {
                if (strncmp(entries.new_values[i], " add", 4) == 0) {
                    osip_message_set_header(sip, entries.headers[i], strchr(entries.new_values[i], ':')+1);
                } 
            }
        }
    }

    if (MSG_IS_OPTIONS(sip)) {
        osip_message_set_header(sip, "X-Timestamp", "11:11:11"); 
    }

    char *dest = NULL; 
    size_t length; 
    osip_message_to_str(sip, &dest, &length);
    strcpy(buffer, dest); 

}

int classify_message(char *buffer, ManipulationTable *table){
    char tmp[1024];
    strcpy(tmp, buffer);
    char *r_uri_end = strstr(tmp, "\r\n"); 
    *r_uri_end = '\0';

    for (int i = 0; i < table->count; i++) {
        if(strncmp(tmp, table->entries[i].message_type, strlen(table->entries[i].message_type)) == 0){
            manipulate_existing_header(buffer, table->entries[i]);
            return 0;
        }
    }
    return 1;     
}

void print_hmr_table(ManipulationTable *table) {
    printf("eingelesene Manipulation-Table: \n");
    for (int i = 0; i < table->count; i++) {
        printf("Message-Type: %s -> Entries: %d\n", table->entries[i].message_type, table->entries[i].count);
        for (int j = 0; j < table->entries[i].count; j++) {
            printf("\tHeader: %s\n", table->entries[i].headers[j]);
            printf("\tNew Value: %s\n", table->entries[i].new_values[j]);
        }
    }
}

int main() {
    char    buffer[1024];
    char    pcscf[] = "217.0.146.197";

    create_sip_option(buffer, pcscf);
    printf("Sizeof buffer: %ld\n", strlen(buffer)); 
    ManipulationTable *table = load_hmr("./project/shared/hmr.txt");
    if (classify_message(buffer, table) == 0) {
        printf("Manipulierte Message:\n%s", buffer);
    }
    
    printf("Keine Manipulation durchgeführt\n"); 
    
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


/*    char path[1024] = "\0";
    int pid = -1;

    FILE *fp = popen("pgrep manipulator", "r"); 
    if (fgets(path, sizeof(path)-1, fp)!=NULL) {
        printf("%s\n", path);
        pid = atoi(path); 
    } else {
        printf("Else-ZWeig");
        pclose(fp);
        return 0;
    }

    printf("%d\n\n", pid);

    pclose(fp);

  if (kill(pid, SIGTERM) == -1) {
        printf("Fehler beim terminieren\nTry to kill\n");
        kill(pid, SIGKILL);
    } else { 
        printf("erfolgreich terminiert\n"); 
    }*/

/*
char    *filepath = "./project/shared/config.txt";
    char    ip_address_ext[16];
    char    ip_port_int[8];
    char    ip_port_ext[8];
    char    main_log[64];
    char    sip_man_log[64];
    char    version[5];

    FILE *config = fopen(filepath, "r"); 
    if (config == NULL) {
        printf("Fehler beim öffnen\n"); 
    }

    char line[128];
    int i = 1; 
    while(fgets(line, sizeof(line), config)) {
        if (line[0] == '#' || line[0] == '\n') {
            i++;
            continue;
        } else {
            if (strncmp(line, "IP_ADDRESS_EXT", 14) == 0) {
                sscanf(line, "IP_ADDRESS_EXT=%s", ip_address_ext);
            } else if (strncmp(line, "IP_PORT_INT", 11) == 0) {
                sscanf(line, "IP_PORT_INT=%s", ip_port_int);
            } else if (strncmp(line, "IP_PORT_EXT", 11) == 0) {
                sscanf(line, "IP_PORT_EXT=%s", ip_port_ext);
            } else if (strncmp(line, "MAIN_LOG", 8) == 0) {
                sscanf(line, "MAIN_LOG=%s", main_log); 
            } else if (strncmp(line, "MAIN_SIP_MAN", 12) == 0) {
                sscanf(line, "MAIN_SIP_MAN=%s", sip_man_log); 
            } else if (strncmp(line, "VERSION", 7) == 0) {
                sscanf(line, "VERSION=%s", version); 
            }
        }
    }

    fclose(config); 
    printf("%s\n", ip_address_ext);
    printf("%s\n", ip_port_ext); 
    printf("%s\n", ip_port_int); 
    printf("%s\n", main_log); 
    printf("%s\n", sip_man_log); 
    printf("Version: %s\n", version); 
*/

/*
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

char* get_timestamp(){
    time_t rawtime; 
    struct tm *timeinfo;
    static char timeString[100];

    time(&rawtime); 
    timeinfo = localtime(&rawtime); 
    strftime(timeString, sizeof(timeString), "%d-%m-%Y %H:%M", timeinfo);

    return timeString;
}

void error_msg(char *logfile, char *error_msg) {
    FILE *file = fopen(logfile, "a"); 
    fprintf(file, "%s - %s\n", get_timestamp(), error_msg);
}

*/