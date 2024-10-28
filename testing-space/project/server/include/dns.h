#ifndef DNS_H
#define DNS_H
#include <stdatomic.h>

extern char        a_record_prio10[64];
extern char        a_record_prio20[64];
extern char        a_record_prio30[64];
extern uint32_t    a_record_ttl;
extern uint32_t    srv_ttl; 

extern char         dns_config_path[128];
extern char         domain[128];
extern char         own_precedense[64];
extern char         sip_man_log[64];

extern volatile atomic_int execute_loop;

void *start_dns_thread(void* args); 


#endif