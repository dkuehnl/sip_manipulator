#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ncurses.h>
#include <stdbool.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

#include "../shared/include/log.h"
#include "./include/process_handling.h"


int get_pid(const char  *name, char *logfile) {
    char    path[265];
    int     pid;
    char    command[265];
    char    error[64];

    snprintf(command, sizeof(command), "pgrep %s", name);

    FILE *fp = popen(command, "r"); 
    if (fgets(path, sizeof(path)-1, fp)!=NULL) {
        pid = atoi(path); 
    } else {
        snprintf(error, sizeof(error), "(PROCESS) INFO: No PID found for %s", name);
        error_msg(logfile, error); 
        return 0;
    }
    snprintf(error, sizeof(error), "(PROCESS) INFO: PID %d found for %s", pid, name);
    error_msg(logfile, error);

    pclose(fp);
    return pid;
}

int terminate_process(const int pid, char *logfile) {
    char    error[64];

    if (kill(pid, SIGTERM) == -1) {
        snprintf(error, sizeof(error), "(PROCESS) WARNING: PID %d could not be terminated, trying to force.", pid);
        error_msg(logfile, error); 
        if (kill(pid, SIGKILL) == -1) {
            snprintf(error, sizeof(error), "(PROCESS) ERROR: PID %d could not be killed.", pid); 
            error_msg(logfile, error);
            return -1;
        } else {
            snprintf(error, sizeof(error), "(PROCESS) INFO: PID %d stopped by SIGKILL.", pid);
            error_msg(logfile, error); 
        }
    } else {
        snprintf(error, sizeof(error), "(PROCESS) INFO: PID %d stopped by SIGTERM.", pid); 
        error_msg(logfile, error); 
    }
    return 0;
}


int start_nano(const char *filename, char *logfile){
    endwin();
    pid_t pid = fork(); 
    char error[64];

    if (pid == -1) {
        error_msg(logfile, "(PROCESS) ERROR: Failed to create child-process.");
        return 1; 
    }

    if (pid == 0) {
        execlp("nano", "nano", filename, (char*)NULL);
        error_msg(logfile, "(PROCESS) INFO: nano started"); 
        perror("exec"); 
        _exit(1);
    } else {
        int status; 
        waitpid(pid, &status, 0);  
        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            return 0;
        } else {
            error_msg(logfile, "(PROCESS) WARNING: Editor closed not correctly, check for data-inconsistence."); 
            return 1;
        }
    }

    refresh(); 
    clear();
    return 0;
}

int start_external_process(const char *choice, const char *filename, char *logfile) {
    char command[128];

    if (strcmp(choice, "SIP") == 0){
        if (!server_running) {
                server_running = true; 
                snprintf(command, sizeof(command), "%s &", filename);
                system(command);
                error_msg(logfile, "(PROCESS) INFO: Server is running..\n");
                return 0;
            } else {
                mvprintw(LINES - 3, 0, "> Server already running\n");
                return 1;
            }
    } else if (strcmp(choice, "HMR") == 0){
        if(start_nano(filename, logfile) == 0) {
            error_msg(logfile, "(PROCESS) INFO: HMR successfully updated\n"); 
            return 0;
        } else {
            error_msg(logfile, "(PROCESS) ERROR: Error while saving new HMR-file\n"); 
            return 1;
        }

    } else if (strcmp(choice, "Config") == 0) {
        if(start_nano(filename, logfile) == 0) {
            error_msg(logfile, "(PROCESS) INFO: Config successfully updated\n"); 
            return 0;
        } else {
            error_msg(logfile, "(PROCESS) ERROR: Error while saving new Config-file\n"); 
            return 1;
        }
    } else {
        error_msg(logfile, "(PROCESS) WARNING: No valid program configured\n");
        return 1;
    }
}

void stop_sip_server(char *logfile) {
    mvprintw(LINES-3, 0, "> Trying to stop Server\n");
    if (server_running) {
        server_running = false; 
        int pid = get_pid("manipulator", logfile);
        if (pid != 0) {
            if (terminate_process(pid, logfile) == 0) {
                error_msg(logfile, "(PROCESS) INFO: Server has stopped.\n"); 
                mvprintw(LINES - 3, 0, "> Server stopped\n");
            } else {
                mvprintw(LINES -3, 0, "> Error while trying to stop the server - more information in the logs.\n");
                return;
            }
        } else {
            error_msg(logfile, "(PROCESS) INFO: No Process found to stop.\n");
            mvprintw(LINES -3, 0, "> Error while trying to stop the server - more information in the logs.\n");
            return;
        }
    } else {
        mvprintw(LINES -3, 0, "> Server currently not running\n"); 
    }
}

int send_sighup(char *logfile) {
    char command[64];
    int pid = get_pid("manipulator", logfile);

    if (pid == 0) {
        error_msg(logfile, "(PROCESS) INFO: No Server-Process (manipulator) found.");
        return -1;
    }

    int result = kill(pid, SIGHUP); 
    if (result == -1) {
        error_msg(logfile, "(PROCESS) ERROR: Failed to send SIGHUP");
        return -1;
    } 

    error_msg(logfile, "(PROCESS) INFO: SIGHUP send"); 
    return 0;
}