#include <time.h>
#include <stdio.h>
#include <unistd.h>

#include "log.h"

char* get_timestamp(){
    time_t rawtime; 
    struct tm *timeinfo;
    static char timeString[100];

    time(&rawtime); 
    timeinfo = localtime(&rawtime); 
    strftime(timeString, sizeof(timeString), "%d-%m-%Y %H:%M", timeinfo);

    return timeString;
}

void check_logfiles(char *files[], int file_counter) {

    for (int i = 0; i < file_counter; i++) {
        if (access(files[i], F_OK) == 0) {
            FILE *fd = fopen(files[i], "a");
            fprintf(fd, "\n\n--------------------------------------------------------------\nApplication started at %s\n\n", get_timestamp()); 
            fclose(fd); 
        } else {
            FILE *fd = fopen(files[i], "w"); 
            fprintf(fd, "Application started at %s\n\n", get_timestamp()); 
            fclose(fd); 
        }
    }
    
}