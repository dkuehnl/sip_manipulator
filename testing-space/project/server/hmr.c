#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <osip2/osip.h>
#include <osipparser2/osip_const.h>
#include <osipparser2/osip_headers.h>
#include <osipparser2/osip_body.h>
#include <osipparser2/osip_message.h>

#include "../shared/include/log.h"
#include "./include/hmr.h"

//manipulate_sdp(sdp_data, hmr_entries, sip_man_log, sip_hmr_log); 
void manipulate_sdp(char *sdp_data, ManipulationEntry hmr_entries, const char *sip_man_log, const char *sip_hmr_log){
    char new_sdp[3060];
    memset(new_sdp, 0, sizeof(new_sdp));

    /*char *line_beginning = strstr(sdp_data, )
    printf("%s", sdp_data);*/ 
}

void process_hmr(char *sip_message, ManipulationEntry hmr_entries, const char *sip_man_log, const char *sip_hmr_log) {
    osip_t *osip;
    if (osip_init(&osip)!=0) {
        error_msg(sip_hmr_log, "(HMR) ERROR process_hmr: Parser couldn't be initialized.");
        error_msg(sip_man_log, "(MHR) ERROR process_hmr: Parser couldn't be initialized.");
        return;
    }

    osip_message_t *sip; 
    osip_message_init(&sip);
    if (osip_message_parse(sip, sip_message, strlen(sip_message))!=0){
        error_msg(sip_hmr_log, "(HMR) ERROR process_hmr: Message couldn't be parsed.");
        error_msg(sip_man_log, "(HMR) ERROR process_hmr: Message couldn't be parsed.");
        return;
    }

    for (int i = 0; i < hmr_entries.count; i++){
        if (strncmp(hmr_entries.headers[i], "sdp", 3) == 0) {
            if(strncmp(hmr_entries.new_values[i], " true", 5) == 0){
                osip_body_t *sdp_body;
                if (sdp_body == NULL){
                    error_msg(sip_hmr_log, "(HMR) ERROR process_hmr: SDP-Body-Element could not be initialized.");
                    continue;
                }
                osip_message_get_body(sip, 0, &sdp_body);
                char *sdp_data = sdp_body->body;
                if (sdp_data == NULL){
                    error_msg(sip_hmr_log, "(HMR) ERROR process_hmr: SDP could not be extracted. No manipulation will be done.");
                    continue;
                }
                manipulate_sdp(sdp_data, hmr_entries, sip_man_log, sip_hmr_log); 
                size_t length = strlen(sdp_data); 
                char length_str[20];
                snprintf(length_str, sizeof(length_str), "%zu", length); 
                osip_list_remove(&sip->bodies, 0);
                osip_message_set_body(sip, sdp_data, strlen(sdp_data));
                osip_message_set_content_length(sip, length_str);
            }

        } else if (strncmp(hmr_entries.headers[i], "From", 4) == 0) {
            if (strncmp(hmr_entries.new_values[i], " host", 5) == 0){
                sip->from->url->host = strchr(hmr_entries.new_values[i], ':')+1;
            } else if (strncmp(hmr_entries.new_values[i], " user", 5) == 0) {
                sip->from->url->username = strchr(hmr_entries.new_values[i], ':')+1;
            } else if (strncmp(hmr_entries.new_values[i], " scheme", 7) == 0){
                sip->from->url->scheme = strchr(hmr_entries.new_values[i], ':')+1;
            }
        } else if (strncmp(hmr_entries.headers[i], "To", 2) == 0) {
            if (strncmp(hmr_entries.new_values[i], " host", 5) == 0){
                sip->to->url->host = strchr(hmr_entries.new_values[i], ':')+1;
            } else if (strncmp(hmr_entries.new_values[i], " user", 5) == 0) {
                sip->to->url->username = strchr(hmr_entries.new_values[i], ':')+1;
            } else if (strncmp(hmr_entries.new_values[i], " scheme", 7) == 0){
                sip->to->url->scheme = strchr(hmr_entries.new_values[i], ':')+1;
            }
        } else if (strncmp(hmr_entries.headers[i], "Call-ID", 7) == 0) {
            if (strncmp(hmr_entries.new_values[i], " host", 5) == 0){
                sip->call_id->host = strchr(hmr_entries.new_values[i], ':')+1;
            } else if (strncmp(hmr_entries.new_values[i], " number", 7) == 0) {
                sip->call_id->number = strchr(hmr_entries.new_values[i], ':')+1;
            }
        } else if (strncmp(hmr_entries.headers[i], "Contact", 7) == 0) {
            osip_contact_t *contact;
            osip_message_get_contact(sip, 0, &contact);
            if (strncmp(hmr_entries.new_values[i], " scheme", 7) == 0){
                contact->url->scheme = strchr(hmr_entries.new_values[i], ':')+1;
            } else if (strncmp(hmr_entries.new_values[i], " user", 5) == 0) {
                contact->url->username = strchr(hmr_entries.new_values[i], ':')+1;
            } else if (strncmp(hmr_entries.new_values[i], " host", 5) == 0) {
                contact->url->host = strchr(hmr_entries.new_values[i], ':')+1;
            } else if (strncmp(hmr_entries.new_values[i], " port", 5) == 0) {
                contact->url->port = strchr(hmr_entries.new_values[i], ':')+1;
            }
        } else if (strncmp(hmr_entries.headers[i], "CSeq", 7) == 0) {
            if (strncmp(hmr_entries.new_values[i], " number", 7) == 0){
                sip->cseq->number = strchr(hmr_entries.new_values[i], ':')+1;
            } else if (strncmp(hmr_entries.new_values[i], " method", 7) == 0) {
                sip->cseq->method = strchr(hmr_entries.new_values[i], ':')+1;
            }
        }  else if (strncmp(hmr_entries.headers[i], "Via", 7) == 0) {
            continue;
        } else {
            osip_header_t *header;
            if (osip_message_header_get_byname(sip, hmr_entries.headers[i], 0, &header) >  0){
                if (strncmp(hmr_entries.new_values[i], " del", 4) == 0) {
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
                    header->hvalue = hmr_entries.new_values[i];
                }
            } else {
                if (strncmp(hmr_entries.new_values[i], " add", 4) == 0) {
                    osip_message_set_header(sip, hmr_entries.headers[i], strchr(hmr_entries.new_values[i], ':')+1);
                } 
            }
        }
    }

    if (MSG_IS_OPTIONS(sip)) {
        osip_message_set_header(sip, "X-Timestamp", get_timestamp()); 
    }
    char *dest = NULL; 
    size_t length; 
    osip_message_to_str(sip, &dest, &length);
    strcpy(sip_message, dest);
    
    //strncpy(sip_message, dest, sizeof(dest)-1);
    //sip_message[sizeof(sip_message)-1] = '\0';
    //free(dest); 
}

 

void process_sip(char *sip_message, ManipulationTable *modification_table, const char *sip_man_log, const char *sip_hmr_log) {
    char tmp[8192];
    char tmp_err_msg[256];
    strcpy(tmp, sip_message);
    char *r_uri_end = strstr(tmp, "\r\n"); 
    *r_uri_end = '\0';

    snprintf(tmp_err_msg, sizeof(tmp_err_msg), "(HMR) INFO: Following Message-Type received: %s, checking for hmr-rules", tmp);
    error_msg(sip_hmr_log, tmp_err_msg); 
    for (int i = 0; i < modification_table->count; i++) {
        if(strncmp(tmp, modification_table->entries[i].message_type, strlen(modification_table->entries[i].message_type)) == 0){
            snprintf(tmp_err_msg, sizeof(tmp_err_msg), "(HMR) INFO: %d Rules found for %s-Messages", modification_table->entries[i].count, modification_table->entries[i].message_type);
            error_msg(sip_hmr_log, tmp_err_msg); 
            process_hmr(sip_message, modification_table->entries[i], sip_man_log, sip_hmr_log);
            return;
        }
    }
    error_msg(sip_hmr_log, "(HMR) INFO: No HMR found, Message processed without manipulation."); 
    error_msg(sip_man_log, "(HMR) INFO: No HMR found, Message processed without manipulation.");   
}