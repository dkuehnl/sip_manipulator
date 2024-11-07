#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include "./include/state_machine.h"
#include "../shared/include/log.h"

const char* request_types[] = {
    "INVITE",
    "ACK",
    "OPTIONS",
    "BYE",
    "CANCEL",
    "REGISTER",
    "SUBSCRIBE",
    "NOTIFY",
    "PUBLISH",
    "INFO",
    "PRACK",
    "UPDATE",
    "REFER",
    "MESSAGE",         
    "CANCEL"
};

void state_idle(StateMachine* sm, int socket_fd, const char* sip_man_log); 
void state_read_socket(StateMachine* sm, int socket_fd, const char* sip_man_log); 
void state_process_buffer(StateMachine* sm, int socket_fd, const char* sip_man_log);
void state_process_sdp(StateMachine* sm, int socket_fd, const char* sip_man_log);
void state_error(StateMachine* sm, int socket_fd, const char* sip_man_log); 
void state_done(StateMachine* sm, int socket_fd, const char* sip_man_log); 
void state_connection_closed(StateMachine* sm, int socket_fd, const char* sip_man_log); 

void init_state_machine(StateMachine* sm) {
    sm->current_state = STATE_IDLE; 
    sm->state_function[STATE_IDLE] = state_idle; 
    sm->state_function[STATE_READ_SOCKET] = state_read_socket;
    sm->state_function[STATE_PROCESS_BUFFER] = state_process_buffer; 
    sm->state_function[STATE_PROCESS_SDP] = state_process_sdp;
    sm->state_function[STATE_ERROR] = state_error; 
    sm->state_function[STATE_DONE] = state_done;
    sm->state_function[STATE_CONNECTION_CLOSED] = state_connection_closed;
    sm->content_length = 0;
    sm->skip_to_sdp = 0;  
}

int extract_sip(const char* buffer, char* full_sip, size_t full_sip_size, const char* sip_man_log){
    const char* sip_end = strstr(buffer, "\r\n\r\n");
    if (sip_end != NULL){
        size_t sip_length = sip_end - buffer +4;
        if ((full_sip_size-strlen(full_sip)) < sip_length){
            error_msg(sip_man_log, "(STATE-MA) ERROR: Buffer-Overflow! Not enough memory available in full_sip.");
            return 1;
        }
        strncat(full_sip, buffer, sip_length);
        return 0;  
    } else {
        strncat(full_sip, buffer, full_sip_size-1);
    }
}

int get_content_length(char* buffer, int* content_length){
    const char* header = "Content-Length: "; 
    char* header_position = strstr(buffer, header); 
    if (header_position != NULL){
        header_position += strlen(header); 
        while (*header_position == ' '){
            header_position++;
        }
    } else {
        return 1;
    }

    *content_length = atoi(header_position);
    return 0;
}

int check_sip_message_begin(char* buffer, const char* sip_man_log){
    //Check for Request-Type
    char tmp[265];
    for (int i = 0; i < 15; i++){
        if (strncmp(buffer, request_types[i], strlen(request_types[i])) == 0){
            snprintf(tmp, sizeof(tmp), "(STATE-MA) INFO: Following Request detected: %s\n", request_types[i]);
            error_msg(sip_man_log, tmp);
            return 1;
        }
    }

    //Check if Response
    if (strncmp(buffer, "SIP/2.0", 7) == 0){
        error_msg(sip_man_log, "(STATE-MA) INFO: Response detected.");
        return 1;
    }
    return 0;
}

int check_sip_message_complete(char* buffer, char* full_sip, const char* sip_man_log){
    if (check_sip_message_begin(buffer, sip_man_log) == 1 || strlen(full_sip) > 0){
        if(strstr(buffer, "\r\n\r\n") != NULL){ 
            return 1; 
        } else {
            return 2;
        }
    } else {
        return 0;
    }
}

void handle_event(StateMachine* sm, int socket_fd, const char* sip_man_log) {
    char tmp[256];
    snprintf(tmp, sizeof(tmp), "(STATE-MA) INFO: Current-State: %d\n", sm->current_state);
    error_msg(sip_man_log, tmp); 
    while (sm->current_state != STATE_IDLE) {
        sm->state_function[sm->current_state](sm, socket_fd, sip_man_log);

        if (sm->current_state == STATE_ERROR || sm->current_state == STATE_CONNECTION_CLOSED) {
            sm->current_state = STATE_CONNECTION_CLOSED;
            error_msg(sip_man_log, "(STATE-MA) INFO: State Close erreicht.");
            break;
        }
    } 
}

void state_idle(StateMachine* sm, int socket_fd, const char* sip_man_log) {
    error_msg(sip_man_log, "(STATE-MA) INFO: Idle-State reached."); 
}

void state_read_socket(StateMachine* sm, int socket_fd, const char* sip_man_log) {
    error_msg(sip_man_log, "(STATE-MA) INFO: read_socket-State");
    sm->buffer[0] = '\0';
    ssize_t rv = read(socket_fd, sm->buffer, sizeof(sm->buffer)-1);
    if (rv>0){
        error_msg(sip_man_log, "(STATE-MA) INFO: Get Data from socket.");
        sm->buffer[rv] = '\0';
        sm->current_state = STATE_PROCESS_BUFFER;
    } else if (rv < 0) {
        error_msg(sip_man_log, "(STATE-MA) ERROR: Something went wrong on socket."); 
        sm->current_state = STATE_ERROR; 
    } else {
        error_msg(sip_man_log, "(STATE-MA) WARNING: Connection closed"); 
        sm->current_state = STATE_CONNECTION_CLOSED;
    }
}

void state_process_buffer(StateMachine* sm, int socket_fd, const char* sip_man_log) {
    char tmp[256];
    error_msg(sip_man_log, "(STATE-MA) INFO: process_buffer-State"); 

    if(sm->skip_to_sdp == 1){
        error_msg(sip_man_log, "(STATE-MA) INFO: Skip to sdp.");
        sm->current_state = STATE_PROCESS_SDP;
    } else {
        int result = check_sip_message_complete(sm->buffer, sm->full_sip, sip_man_log);
        if ( result == 1){
            error_msg(sip_man_log, "(STATE-MA) INFO: Beginning and end of SIP-message contained in Buffer");
            if(extract_sip(sm->buffer, sm->full_sip, sizeof(sm->full_sip), sip_man_log) == 1){
                sm->current_state = STATE_ERROR;
                return;
            }
            if(get_content_length(sm->buffer, &sm->content_length) == 1){
                error_msg(sip_man_log, "(STATE-MA) ERROR: Mailformed SIP-message, no content-length in complete sip-message detected.");
                sm->current_state = STATE_ERROR; 
                return;
            }
            if (sm->content_length > 0){
                snprintf(tmp, sizeof(tmp), "(STATE-MA) INFO: SDP detected, Content-Length: %d", sm->content_length);
                error_msg(sip_man_log, tmp); 
                sm->skip_to_sdp = 1;
                sm->current_state = STATE_PROCESS_SDP;
            } else {
                sm->current_state = STATE_IDLE;
            }
        } else if (result == 2) {
            error_msg(sip_man_log, "(STATE-MA) INFO: SIP-Start-Line detected, but no CRLF -> Message incomplete.");
            if (extract_sip(sm->buffer, sm->full_sip, sizeof(sm->full_sip), sip_man_log) == 1){
                error_msg(sip_man_log, "(STATE-MA) ERROR: Extracting sip failed.");
                sm->current_state = STATE_ERROR; 
                return;
            }
            sm->current_state = STATE_READ_SOCKET; 
            
        } else {
            error_msg(sip_man_log, "(STATE-MA) ERROR: No message-Start detected.");
            sm->current_state = STATE_ERROR;
        }
    }
    
}

void state_process_sdp(StateMachine* sm, int socket_fd, const char* sip_man_log) {
    error_msg(sip_man_log, "(STATE-MA) INFO: start sdp-process"); 
    char tmp_log_msg[256];
    char *tmp = strstr(sm->buffer, "\r\n\r\n"); 
    char *sdp_start;
    if (tmp != NULL) {
        sdp_start = tmp +4;
    } else {
        sdp_start = sm->buffer; 
    }

    char *end; 
    while ((end = strstr(sdp_start, "\r\n")) != NULL) {
        int line_length = end - sdp_start +2;
        char temp[1024];
        strncpy(temp, sdp_start, (size_t)line_length);
        temp[line_length] = '\0';
        strncat(sm->full_sdp, temp, sizeof(sm->full_sdp)-strlen(sm->full_sdp)-1);
        sm->content_length -= line_length;
        sdp_start = end + 2;
    }
    if(strlen(sdp_start) > 0){
        strncat(sm->full_sdp, sdp_start, sizeof(sm->full_sdp)-strlen(sm->full_sdp)-1);
    }

    if(sm->content_length > 0){
        snprintf(tmp_log_msg, sizeof(tmp_log_msg), "(STATE-MA) INFO: Remaining Content-Length > 0: %ld", sm->content_length);
        error_msg(sip_man_log, tmp_log_msg);
        sm->current_state = STATE_READ_SOCKET;
    } else if (sm->content_length == 0){
        snprintf(tmp_log_msg, sizeof(tmp_log_msg), "(STATE-MA) INFO: Message complete. Remaining Content-Length = 0: %ld", sm->content_length);
        error_msg(sip_man_log, tmp_log_msg);
        sm->current_state = STATE_DONE;
    } else {
        snprintf(tmp_log_msg, sizeof(tmp_log_msg), "(STATE-MA) ERROR: Invalid Content-Length: %ld", sm->content_length); 
        error_msg(sip_man_log, tmp_log_msg);
        sm->current_state = STATE_ERROR;
    }
}

void state_error(StateMachine* sm, int socket_fd, const char* sip_man_log) {
    error_msg(sip_man_log, "(STATE-MA) INFO: reached error-State");
    close(socket_fd); 
    sm->skip_to_sdp = 0;
    sm->current_state = STATE_IDLE;
}

void state_done(StateMachine* sm, int socket_fd, const char* sip_man_log) {
    error_msg(sip_man_log, "(STATE-MA) INFO: reached done-State"); 
    sm->skip_to_sdp = 0;
    sm->current_state = STATE_IDLE;
}

void state_connection_closed(StateMachine* sm, int socket_fd, const char* sip_man_log) {
    error_msg(sip_man_log, "(STATE-MA) INFO: Connection Closed");
    close(socket_fd); 
    sm->skip_to_sdp = 0;
}