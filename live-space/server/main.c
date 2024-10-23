#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <sys/time.h>
#include <signal.h>
#include <pthread.h>

#include "../shared/include/log.h"
#include "./include/hmr.h"
#include "./include/dns.h"

#define GLOBAL_CONFIG_PATH "/usr/src/app/config.txt"

//Globale Filepaths: 
char                int_hmr_path[64];
char                ext_hmr_path[64];
char                mir_hmr_path[64];
char                sip_man_log[64];
char                sip_hmr_log[64];
char                dns_config_path[128] = {0};
ManipulationTable   *int_modification_table = NULL;
ManipulationTable   *ext_modification_table = NULL;
ManipulationTable   *mir_modification_table = NULL;

char                domain[128];
char                own_precedense[64];     
int                 sockfd, connfd, sockfd_ext;

//Function-Declaration:
ManipulationTable *load_hmr(const char *hmr_path, char *sip_man_log){ 
    ManipulationTable *table = malloc(sizeof(ManipulationTable));
    table->entries = NULL;
    table->count = 0;

    FILE *hmr = fopen(hmr_path, "r"); 
    if (hmr == NULL) {
        error_msg(sip_man_log, "(MAIN) ERROR: Error while reading file");
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

void free_manipulation_table(ManipulationTable *table) {
    if (table) {
        for (int i = 0; i < table->count; i++){
            free(table->entries[i].message_type); 
            for (int j = 0; j < table->entries[i].count; j++){
                free(table->entries[i].headers[j]);
                free(table->entries[i].new_values[j]);
            }
            free(table->entries[i].headers); 
            free(table->entries[i].new_values); 
        }
        free(table->entries); 
        free(table); 
    }   
}

void load_main_config(char *version, char *sip_man_log, char *sip_hmr_log, char *int_hmr_path, char *ext_hmr_path, 
char *mir_hmr_path, char *ip_address_ext, char *ip_port_ext, char *ip_port_int, int *mirror, char *dns_config_path, 
char *domain, char *own_precedense){
    FILE *config = fopen(GLOBAL_CONFIG_PATH, "r");
    if (config == NULL) {
        printf("(MAIN) CRITIC: Config-File could not be opened!\n");
        exit(EXIT_FAILURE); 
    }

    char line[128];
    while (fgets(line, sizeof(line), config)) {
        if (strncmp(line, "IP_ADDRESS_EXT", 14) == 0) {
            sscanf(line, "IP_ADDRESS_EXT=%16s", ip_address_ext);
        }else if (strncmp(line, "IP_PORT_INT", 11) == 0) {
            sscanf(line, "IP_PORT_INT=%6s", ip_port_int);
        }else if (strncmp(line, "IP_PORT_EXT", 11) == 0) {
            sscanf(line, "IP_PORT_EXT=%6s", ip_port_ext);
        }else if (strncmp(line, "MAIN_SIP_LOG", 12) == 0) {
            sscanf(line, "MAIN_SIP_LOG=%64s", sip_man_log); 
        } else if (strncmp(line, "HMR_LOG", 7) == 0) {
            sscanf(line, "HMR_LOG=%64s", sip_hmr_log);   
        } else if (strncmp(line, "VERSION", 7) == 0) {
            sscanf(line, "VERSION=%8s", version); 
        } else if (strncmp(line, "INC_HMR_PATH", 12) == 0) {
            sscanf(line, "INC_HMR_PATH=%64s", int_hmr_path);
        } else if (strncmp(line, "OUT_HMR_PATH", 12) == 0) {
            sscanf(line, "OUT_HMR_PATH=%64s", ext_hmr_path);  
        } else if (strncmp(line, "MIR_HMR_PATH", 12) == 0) {
            sscanf(line, "MIR_HMR_PATH=%64s", mir_hmr_path); 
        } else if (strncmp(line, "MIRROR", 6) == 0) {
            sscanf(line, "MIRROR=%1d", mirror);
        } else if (strncmp(line, "RESOLVE_FILE", 12) == 0) {
            sscanf(line, "RESOLVE_FILE=%127s", dns_config_path);
        } else if (strncmp(line, "DOMAIN", 6) == 0) {
            sscanf(line, "DOMAIN=%128s", domain);
        } else if (strncmp(line, "OWN_PREC", 8) == 0) {
            sscanf(line, "OWN_PREC=%64s", own_precedense);
        }
    }
    fclose(config);
}

//Signal-Handler: 
void sighandler_mirror() {
    error_msg(sip_man_log, "(MAIN) INFO: Incoming SIGHUP, reloading HMR");
    mir_modification_table = load_hmr(mir_hmr_path, sip_man_log); 
    if (mir_modification_table == NULL) {
        error_msg(sip_man_log, "(MAIN) ERROR: MIR_HMR could not be loaded."); 
    }
    error_msg(sip_man_log, "(MAIN) INFO: HMR reloaded finished");
}

void sighandler_no_mirror() {
    error_msg(sip_man_log, "(MAIN) INFO: Incoming SIGHUP, reloading HMR");
    int_modification_table = load_hmr(int_hmr_path, sip_man_log);
    if (int_modification_table == NULL) {
        error_msg(sip_man_log, "(MAIN) ERROR: INC_HMR could not be loaded."); 
    }
    ext_modification_table = load_hmr(ext_hmr_path, sip_man_log); 
    if (ext_modification_table == NULL) {
        error_msg(sip_man_log, "(MAIN) ERROR: OUT_HMR could not be loaded."); 
    }
    error_msg(sip_man_log, "(MAIN) INFO: HMR reloaded finished"); 
}

void handle_sigterm() {
    error_msg(sip_man_log, "(MAIN) INFO: Received SIGTERM, waiting 30 seconds before closing everything."); 
    sleep(30);

    error_msg(sip_man_log, "(MAIN) INFO: Closing all sockets.");
    close(connfd); 
    close(sockfd); 
    close(sockfd_ext);
    free_manipulation_table(int_modification_table);
    free_manipulation_table(ext_modification_table); 
    free_manipulation_table(mir_modification_table);

    error_msg(sip_man_log, "(MAIN) INFO: Terminating Server by SIGTERM"); 
    exit(0); 
}


int main()
{
    struct sockaddr_in  sockaddr, connaddr, sockaddr_ext;
    unsigned int        connaddr_len;
    char                buffer[2048];
    int                 rv, rv_ext;
    struct sigaction    sa;
    int                 result;
    pthread_t           dns_thread;

    //Default-Values (overwritten by config-file)
    char                ip_addr_ext[16] = "127.0.0.1";
    char                ip_port_ext[6]  = "10000";
    char                ip_port_int[6]  = "5060";
    char                version[8];
    char                tmp[2048];
    int                 mirror = 0;  

    if(access(GLOBAL_CONFIG_PATH, (F_OK | R_OK)) != 0) {
        printf("(MAIN) CRITIC: No Config-File found - please make sure it in place (%s)!\n", GLOBAL_CONFIG_PATH);
        exit(EXIT_FAILURE); 
    }

    load_main_config(
        version, 
        sip_man_log, 
        sip_hmr_log, 
        int_hmr_path, 
        ext_hmr_path, 
        mir_hmr_path, 
        ip_addr_ext, 
        ip_port_ext, 
        ip_port_int, 
        &mirror, 
        dns_config_path, 
        domain, 
        own_precedense); 

    //Start Logging
    snprintf(tmp, sizeof(tmp), "SIP-Manipulator v%s startet.", version);
    error_msg(sip_man_log, tmp);
    error_msg(sip_man_log, "Loading config-file..."); 
    snprintf(tmp, sizeof(tmp), "Following values are used:\n\texternal IP: %s\texternal Port: %s\tinternal Port: '%s'", ip_addr_ext, ip_port_ext, ip_port_int);
    error_msg(sip_man_log, tmp);
    if (mirror == 1){
        error_msg(sip_man_log, "(MAIN) INFO: Acting as Mirror");
        sa.sa_handler = sighandler_mirror;
        sa.sa_flags = 0; 
        sigemptyset(&sa.sa_mask);
        if (sigaction(SIGHUP, &sa, NULL) == -1){
            error_msg(sip_man_log, "(MAIN) ERROR: SIG-Handler could not be initialized.");
            exit(EXIT_FAILURE); 
        } else {
            error_msg(sip_man_log, "(MAIN) INFO: SIG-Handler established.");
        }
    } else if (mirror == 0){
        error_msg(sip_man_log, "(MAIN) INFO: Acting as Proxy"); 
        sa.sa_handler = sighandler_no_mirror;
        sa.sa_flags = 0; 
        sigemptyset(&sa.sa_mask); 
        if (sigaction(SIGHUP, &sa, NULL) == -1){
            error_msg(sip_man_log, "(MAIN) ERROR: SIG-Handler could not be initialized."); 
            exit(EXIT_FAILURE); 
        } else {
            error_msg(sip_man_log, "(MAIN) INFO: SIG-Handler established."); 
        }

        if(pthread_create(&dns_thread, NULL, start_dns_thread, NULL) != 0){
            error_msg(sip_man_log, "(MAIN) WARNING: Error while creating DNS-Thread.");
        };
    }
    signal(SIGTERM, handle_sigterm); 

    //create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        error_msg(sip_man_log, "(MAIN) ERROR: Socket creation failed");
        exit(EXIT_FAILURE);
    }

    //set IP address & IP port
    memset(&sockaddr, 0x00, sizeof(sockaddr));
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    sockaddr.sin_port = htons(atoi(ip_port_int));

    //bind socket
    if((bind(sockfd, (struct sockaddr*)&sockaddr, sizeof(sockaddr))) != 0) {
        error_msg(sip_man_log,"(MAIN) ERROR: Socket bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    //listen to socket
    if((listen(sockfd, 5)) != 0) {
        error_msg(sip_man_log, "(MAIN) ERROR: Listen failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    else {
        error_msg(sip_man_log, "(MAIN) INFO: Server listening");
    }

    //Load HMR's
    if (mirror == 0) {
        int_modification_table = load_hmr(int_hmr_path, sip_man_log);
        if (int_modification_table == NULL) {
            error_msg(sip_man_log, "(MAIN) ERROR: INC_HMR could not be loaded."); 
        }
        ext_modification_table = load_hmr(ext_hmr_path, sip_man_log); 
        if (ext_modification_table == NULL) {
            error_msg(sip_man_log, "(MAIN) ERROR: OUT_HMR could not be loaded."); 
        }
    } else if (mirror == 1) {
        mir_modification_table = load_hmr(mir_hmr_path, sip_man_log); 
        if (mir_modification_table == NULL) {
            error_msg(sip_man_log, "(MAIN) ERROR: MIR_HMR could not be loaded."); 
        }
    } else {
        error_msg(sip_man_log, "(MAIN) ERROR: No HMR-File loaded, shutdown now.");
        close(sockfd);
        exit(EXIT_FAILURE);
    }


    //server loop
    while(1)
    {
        error_msg(sip_man_log, "(MAIN) INFO: Waiting for incoming connection."); 
        connaddr_len = sizeof(connaddr);
        connfd = accept(sockfd, (struct sockaddr*)&connaddr, &connaddr_len);
        if(connfd < 0){
            error_msg(sip_man_log, "(MAIN) ERROR 1ST-CON: Something went wrong with accepting the socket.");
        } else {
            error_msg(sip_man_log, "(MAIN) INFO: New connection accepted.");
        }

        //2nd-Connection
        if (mirror == 0) {
            sockfd_ext = socket(AF_INET, SOCK_STREAM, 0);
            if (sockfd_ext == -1) {
                error_msg(sip_man_log, "(MAIN) ERROR 2ND-CON: Creating of externel Socket failed.");
                break;
            }

            memset(&sockaddr_ext, 0x00, sizeof(sockaddr_ext));
            sockaddr_ext.sin_family = AF_INET;
            sockaddr_ext.sin_addr.s_addr = inet_addr(ip_addr_ext);
            sockaddr_ext.sin_port = htons(atoi(ip_port_ext));

            if(connect(sockfd_ext, (struct sockaddr*)&sockaddr_ext, sizeof(sockaddr_ext)) != 0) {
                error_msg(sip_man_log, "(MAIN) ERROR 2ND-CON: Connect to external server failed.");
                close(sockfd_ext); 
                close(connfd);
                close(sockfd); 
                exit(EXIT_FAILURE);
            } else {
                snprintf(tmp, sizeof(tmp), "(MAIN) INFO: Connection to %s established successfully", ip_addr_ext);
                error_msg(sip_man_log, tmp);
            }
        }

        //connection loop
        while(1)
        {
            //try to read from socket
            rv = read(connfd, buffer, sizeof(buffer));

            //close connection if error detected
            if(rv < 1){
                error_msg(sip_man_log, "(MAIN) INFO: Connection closed.");
                close(connfd);
                close(sockfd_ext);
                break;
            } else {
                snprintf(tmp, sizeof(tmp), "(MAIN) INFO: %i bytes of data received.", rv);
                error_msg(sip_man_log, tmp);
            }

            if (mirror == 0) {
                process_buffer(buffer, int_modification_table, sip_man_log, sip_hmr_log);
                rv_ext = write(sockfd_ext, buffer, strlen(buffer));
                snprintf(tmp, sizeof(tmp), "(MAIN) INFO: Transmitted Buffer:\n%s", buffer);
                error_msg(sip_hmr_log, tmp);
                memset(buffer, 0, sizeof(buffer));

                rv_ext = read(sockfd_ext, buffer, sizeof(buffer));
                snprintf(tmp, sizeof(tmp), "(MAIN) INFO: %i bytes of data received from external server", rv_ext);
                error_msg(sip_man_log, tmp);
                
                process_buffer(buffer, ext_modification_table, sip_man_log, sip_hmr_log);
                rv = write(connfd, buffer, sizeof(buffer));
                snprintf(tmp, sizeof(tmp), "(MAIN) INFO: Transmitted Buffer:\n%s", buffer);
                error_msg(sip_hmr_log, tmp); 
            } else if (mirror == 1) {
                process_buffer(buffer, mir_modification_table, sip_man_log, sip_hmr_log);
                snprintf(tmp, sizeof(tmp), "(MAIN) INFO Mirror: MIR-Modification applied:\n'%s'", buffer);
                error_msg(sip_hmr_log, tmp); 
                rv = write(connfd, buffer, sizeof(buffer));
            }
        }
    }

    return 0;
}