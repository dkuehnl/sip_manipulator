#ifndef DNS_H
#define DNS_H

extern char        a_record_prio10[64];
extern char        a_record_prio20[64];
extern char        a_record_prio30[64];
extern uint32_t    a_record_ttl;
extern uint32_t    srv_ttl; 

void dns(const char *dns_config_path, const char *domain, char *own_precedense, const char *logfile);


#endif