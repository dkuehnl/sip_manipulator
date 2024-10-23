#include <time.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <string.h>

#include "include/log.h"

char* get_timestamp(){
    struct timeval tv;
    struct tm *timeinfo;
    static char timeString[100];
    char msec_buffer[8];
    gettimeofday(&tv, NULL);
    timeinfo = localtime(&tv.tv_sec);

    strftime(timeString, sizeof(timeString), "%d-%m-%Y %H:%M:%S", timeinfo);
    snprintf(msec_buffer, sizeof(msec_buffer), ".%03ld", tv.tv_usec / 1000);
    strcat(timeString, msec_buffer);

    return timeString;
}


void error_msg(const char *logfile, char *error_msg) {
    FILE *file = fopen(logfile, "a"); 
    fprintf(file, "%s - %s\n", get_timestamp(), error_msg);

    fclose(file); 
}

void check_logfile(char *filepath) {
    if (access(filepath, F_OK) == 0) {
        FILE *fd = fopen(filepath, "a");
        fprintf(fd, "\n\n--------------------------------------------------------------\nApplication started at %s\n\n", get_timestamp()); 
        fclose(fd); 
    } else {
        FILE *fd = fopen(filepath, "w"); 
        if (fd == NULL) {;
            return;
        }
        fprintf(fd, "Application started at %s\n\n", get_timestamp()); 
        fclose(fd); 
    }
    
}

int check_file(char *filepath, char *logfile) {
    if(access(filepath, (F_OK | R_OK)) != 0) {
        char error[64];
        snprintf(error, sizeof(error), "WARNING: '%s' does not exist or got wrong permission", filepath);
        error_msg(logfile, error);
    }

    return 0;
}

