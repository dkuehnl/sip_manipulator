#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

typedef enum {
    STATE_IDLE, 
    STATE_READ_SOCKET, 
    STATE_PROCESS_BUFFER,
    STATE_PROCESS_SDP, 
    STATE_ERROR, 
    STATE_DONE, 
    STATE_CONNECTION_CLOSED,
    STATE_MAX
} State;


typedef struct StateMachine StateMachine;
typedef void (*StateFunction)(StateMachine* sm, int socket_fd, const char* sip_man_log); 

struct StateMachine{
    State current_state; 
    StateFunction state_function[STATE_MAX];
    char buffer[2048];
    char full_sip[3060];
    char full_sdp[3060];
    int content_length;
    int skip_to_sdp;
};

void handle_event(StateMachine* sm, int socket_fd, const char* sip_man_log);
void init_state_machine(StateMachine* sm);

#endif