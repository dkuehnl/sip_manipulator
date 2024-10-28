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

void process_hmr(char *sip_message, ManipulationEntry hmr_entries, char *sip_man_log, char *sip_hmr_log) {
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
        if (strncmp(hmr_entries.headers[i], "From", 4) == 0) {
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
}

 

void process_buffer(char *sip_message, ManipulationTable *modification_table,char *sip_man_log, char *sip_hmr_log) {
    char tmp[2048];
    char tmp_err_msg[128];
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