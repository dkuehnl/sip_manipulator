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

void process_sip(char *sip_message, ManipulationTable *modification_table, const char *sip_man_log, const char *sip_hmr_log);

#endif