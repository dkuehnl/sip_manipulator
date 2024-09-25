#include <stdio.h>
#include <string.h>

#include "../shared/include/log.h"
#include "./include/hmr.h"

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

void process_buffer(char *sip_message, ManipulationTable *modification_table, char *sip_man_log) {
    char tmp[1024];
    char tmp_err_msg[128];
    strcpy(tmp, sip_message);
    char *r_uri_end = strstr(tmp, "\r\n"); 
    *r_uri_end = '\0';

    snprintf(tmp_err_msg, sizeof(tmp_err_msg), "Following Message-Type received: %s, checking for hmr-rules", tmp);
    error_msg(sip_man_log, tmp_err_msg); 
    for (int i = 0; i < modification_table->count; i++) {
        if(strncmp(tmp, modification_table->entries[i].message_type, strlen(modification_table->entries[i].message_type)) == 0){
            snprintf(tmp_err_msg, sizeof(tmp_err_msg), "%d Rules found for %s-Messages", modification_table->entries[i].count, modification_table->entries[i].message_type);
            error_msg(sip_man_log, tmp_err_msg); 
            return;
        }
    }
    error_msg(sip_man_log, "No HMR found");   
}

void classirfy_message(char *sip_message) {

}