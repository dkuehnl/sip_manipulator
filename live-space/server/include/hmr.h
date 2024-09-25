#ifndef HMR_H
#define HMR_H

typedef struct {
    char *message_type;
    char **headers;
    char **new_values;
    int count;
} ManipulationEntry;

typedef struct {
    ManipulationEntry *entries;
    int count;
} ManipulationTable;

void process_buffer(char *sip_message, ManipulationTable *modification_table, char *sip_man_log);

#endif