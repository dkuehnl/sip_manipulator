#ifndef PROCESS_HANDLING_H
#define PROCESS_HANDLING_H

#include <stdio.h>
#include <stdbool.h>

extern bool server_running;

int get_pid(const char  *name, char *logfile);
int terminate_process(const int pid, char *logfile);
int start_nano(const char *filename, char *logfile);
int start_external_process(const char *choice, const char *filename, char *logfile);
void stop_sip_server(char *logfile);



#endif