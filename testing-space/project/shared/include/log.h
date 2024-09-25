#ifndef LOG_H
#define LOG_H

char* get_timestamp();
void check_logfile(char *filepath);
int check_file(char *filepath, char *logfile);
void error_msg(char *logfile, char *error_msg);

#endif