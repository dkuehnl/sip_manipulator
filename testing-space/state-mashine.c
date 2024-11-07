#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>




typedef enum {
    STATE_IDLE, 
    STATE_READ_SOCKET, 
    STATE_PROCESS_BUFFER,
    STATE_PROCESS_SDP, 
    STATE_ERROR, 
    STATE_DONE, 
    STATE_MAX
} State;

typedef enum {
    EVENT_START, 
    EVENT_PROCESS,
    EVENT_SDP_DETECTED,
    EVENT_CONTINUE,
    EVENT_FINISH, 
    EVENT_ERROR, 
    EVENT_RESET, 
    EVENT_MAX
} Event;

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

typedef struct StateMachine StateMachine;
typedef void (*StateFunction)(StateMachine* sm, char* buffer, char* full_message, char* full_sdp, int* content_length, int* fd); 

struct StateMachine{
    State current_state; 
    StateFunction state_function[STATE_MAX];
};

void state_idle(StateMachine* sm, char* buffer, char* full_message, char* full_sdp, int* content_length, int* fd); 
void state_read_socket(StateMachine* sm, char* buffer, char* full_message, char* full_sdp, int* content_length, int* fd); 
void state_process_buffer(StateMachine* sm, char* buffer, char* full_message, char* full_sdp, int* content_length, int* fd);
void state_process_sdp(StateMachine* sm, char* buffer, char* full_message, char* full_sdp, int* content_length, int* fd);
void state_error(StateMachine* sm, char* buffer, char* full_message, char* full_sdp, int* content_length, int* fd); 
void state_done(StateMachine* sm, char* buffer, char* full_message, char* full_sdp, int* content_length, int* fd); 

void init_state_machine(StateMachine* sm) {
    sm->current_state = STATE_IDLE; 
    sm->state_function[STATE_IDLE] = state_idle; 
    sm->state_function[STATE_READ_SOCKET] = state_read_socket;
    sm->state_function[STATE_PROCESS_BUFFER] = state_process_buffer; 
    sm->state_function[STATE_PROCESS_SDP] = state_process_sdp;
    sm->state_function[STATE_ERROR] = state_error; 
    sm->state_function[STATE_DONE] = state_done;
}

State transition_table[STATE_MAX][EVENT_MAX] = {
    //                              EVENT_START         EVENT_PROCESS       EVENT_SDP_DETECTED      EVENT_CONTINUE      EVENT_FINISH        EVENT_ERROR     EVENT_RESET
    [STATE_IDLE] =              {STATE_READ_SOCKET, STATE_MAX, STATE_MAX, STATE_MAX, STATE_IDLE, STATE_ERROR, STATE_IDLE},
    [STATE_READ_SOCKET] =       {STATE_READ_SOCKET, STATE_PROCESS_BUFFER, STATE_MAX, STATE_MAX, STATE_DONE, STATE_ERROR, STATE_IDLE}, 
    [STATE_PROCESS_BUFFER] =    {STATE_MAX, STATE_PROCESS_BUFFER, STATE_PROCESS_SDP, STATE_READ_SOCKET, STATE_DONE, STATE_ERROR, STATE_IDLE}, 
    [STATE_PROCESS_SDP] =       {STATE_MAX, STATE_MAX, STATE_PROCESS_SDP, STATE_READ_SOCKET, STATE_DONE, STATE_ERROR, STATE_IDLE},
    [STATE_ERROR] =             {STATE_ERROR, STATE_ERROR, STATE_ERROR, STATE_ERROR, STATE_ERROR, STATE_ERROR, STATE_IDLE}, 
    [STATE_DONE] =              {STATE_DONE, STATE_DONE, STATE_DONE, STATE_DONE, STATE_DONE, STATE_ERROR, STATE_IDLE}
};

void handle_event(StateMachine* sm, Event event, char* buffer, char* full_message, char* full_sdp, int* content_length, int* fd) {
    printf("Current-State: %d\n", sm->current_state);
    printf("Current Event: %d\n", event);
    State nextState = transition_table[sm->current_state][event];
    if (nextState != STATE_MAX) {
        sm->current_state = nextState; 
        sm->state_function[nextState](sm, buffer, full_message, full_sdp, &content_length, &fd); 
    } else {
        printf("Ungültiges Event %d für den Current_state %d\n", event, sm->current_state); 
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
    }

    content_length = atoi(header_position);
    printf("%d\n", content_length);
    return 0;
}

int check_sip_message_begin(char* buffer){
    //Check for Request-Type
    for (int i = 0; i < 15; i++){
        if (strncmp(buffer, request_types[i], strlen(request_types[i])) == 0){
            printf("Following Request detected: %s\n", request_types[i]);
            return 1;
        }
    }

    //Check if Response
    if (strncmp(buffer, "SIP/2.0", 7) == 0){
        return 1;
    }
    return 0;
}

int check_sip_message_complete(char* buffer){
    if (check_sip_message_begin(buffer) == 1){
        printf("Request-Type detected\n");
        if(strstr(buffer, "\\r\\n\\r\\n") != NULL){ //Hinweis: Suchmuster muss für Prod geändert werden
            printf("CRLF detected\n");
            return 1; 
        } else {
            return 2;
        }
    } else {
        return 0;
    }
}

void state_idle(StateMachine* sm, char* buffer, char* full_message, char* full_sdp, int* content_length, int* fd) {
    printf("Idle-State\n"); 
    return;
}

void state_read_socket(StateMachine* sm, char* buffer, char* full_message, char* full_sdp, int* content_length, int* fd) {
    printf("read_socket-State\n");
    ssize_t rv = read(fd, buffer, sizeof(buffer)-1);
    buffer[rv] = '\0';
    if (rv > 0){
        handle_event(sm, EVENT_PROCESS, buffer, full_message, full_sdp, &content_length, &fd);
    } else if (rv < 0) {
        handle_event(sm, EVENT_ERROR, buffer, full_message, full_sdp, &content_length, &fd);
    } else if (rv == 0) {
        printf("End of File\n");
        //printf("Full Message: \n%s\n", full_message); 
    }
}

void state_process_buffer(StateMachine* sm, char* buffer, char* full_message, char* full_sdp, int* content_length, int* fd) {
    printf("process_buffer-State\n"); 
    if (check_sip_message_complete(buffer) == 1){
        printf("Beginning and End of SIP-Message contained in Buffer\n");
        get_content_length(buffer, content_length); 
        if (content_length > 0){
            printf("SDP detected\n");
            handle_event(sm, EVENT_SDP_DETECTED, buffer, full_message, full_sdp, &content_length, &fd);
        } else {
            handle_event(sm, EVENT_ERROR, buffer, full_message, full_sdp, &content_length, &fd);
        }
    }
    if(content_length != 0){
        handle_event(sm, EVENT_SDP_DETECTED, buffer, full_message, full_sdp, &content_length, &fd);
    }
    strcat(full_message, buffer);

    handle_event(sm, EVENT_CONTINUE, buffer, full_message, full_sdp, &content_length, &fd);
}

void state_process_sdp(StateMachine* sm, char* buffer, char* full_message, char* full_sdp, int* content_length, int* fd) {
    printf("start sdp-process\n"); 
    //Für Prod ebenfalls anpassen
    char *tmp = strstr(buffer, "\\r\\n\\r\\n"); 
    char *sdp_start;
    if (tmp != NULL) {
        sdp_start = tmp +9;
    } else {
        sdp_start = buffer; 
    }

    char *end; 
    while ((end = strstr(sdp_start, "\\r\\n")) != NULL) {
        int line_length = end - sdp_start +4;
        char temp[1024];
        strncpy(temp, sdp_start, (size_t)line_length);
        temp[line_length] = '\0';
        strncat(full_sdp, temp, sizeof(full_sdp)-strlen(full_sdp)-1);
        content_length -= line_length;
        sdp_start = end + 4;
    }
    if(strlen(sdp_start) > 0){
        strncat(full_sdp, sdp_start, sizeof(full_sdp)-strlen(full_sdp)-1);
    }
    
    content_length -= 8;
    if(content_length > 0){
        handle_event(sm, EVENT_CONTINUE, buffer, full_message, full_sdp, &content_length, &fd);
    } else if (content_length == 0){
        handle_event(sm, EVENT_FINISH, buffer, full_message, full_sdp, &content_length, &fd);
    } else {
        handle_event(sm, EVENT_ERROR, buffer, full_message, full_sdp, &content_length, &fd);
    }
}

void state_error(StateMachine* sm, char* buffer, char* full_message, char* full_sdp, int* content_length, int* fd) {
    printf("error-State\n");
}

void state_done(StateMachine* sm, char* buffer, char* full_message, char* full_sdp, int* content_length, int* fd) {
    printf("done-State\n"); 
    printf("Following Buffer for SDP available:\n%s\n", full_sdp);
    return;
}

int main() {
    StateMachine sm; 
    init_state_machine(&sm); 
    char buffer[1024];
    char full_message[2048];
    char full_sdp[2048];
    int fd;
    int content_length;

    fd = open("message.txt", O_RDONLY);
    
    while(1){
        if (sm.current_state == STATE_IDLE) {
            printf("Warte auf neues EVENT.\n");
            getchar(); 
            handle_event(&sm, EVENT_START, buffer, full_message, full_sdp, &content_length, &fd); 
        } else {
            handle_event(&sm, EVENT_RESET, buffer, full_message, full_sdp, &content_length, &fd);
        }
    } 

    close(fd); 
    return 0;
}