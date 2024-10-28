#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <ldns/ldns.h>
#include <signal.h>

#include "../shared/include/log.h"
#include "./include/dns.h"

//Globale Variablen
char        naptr_replacement[128] = {0};

//uint32_t    srv_ttl = 0; 
char        srv_prio10_target[64] = {0};
char        srv_prio20_target[64] = {0};
char        srv_prio30_target[64] = {0};

uint32_t    a_record_ttl = 0;
uint32_t    srv_ttl = 0; 
char        a_record_prio10[64] = {0};
char        a_record_prio20[64] = {0};
char        a_record_prio30[64] = {0};

uint16_t    flags = 0; 


void get_naptr(const char *dns_config_path, const char *dns_name, const char *precedense){
    ldns_resolver   *resolver; 
    ldns_pkt        *packet;
    ldns_rdf        *domain;
    ldns_rr_list    *response;
    ldns_status     status;
    char            naptr_service[32] = {0};

    domain = ldns_dname_new_frm_str(dns_name);
    status = ldns_resolver_new_frm_file(&resolver, dns_config_path); 
    packet = ldns_resolver_search(resolver, domain, LDNS_RR_TYPE_NAPTR, LDNS_RR_CLASS_IN, flags); 

    response = ldns_pkt_rr_list_by_type(packet, LDNS_RR_TYPE_NAPTR, LDNS_SECTION_ANSWER);
    for (size_t i = 0; i < ldns_rr_list_rr_count(response); i++){
        ldns_rr *rr = ldns_rr_list_rr(response, i); 
        strcpy(naptr_service, ldns_rdf2str(ldns_rr_rdf(rr, 3)));
        if (strncmp(naptr_service, precedense, 7) == 0){
            strcpy(naptr_replacement, ldns_rdf2str(ldns_rr_rdf(rr, 5)));
            continue;
        }
    }

    ldns_rr_list_deep_free(response); 
    ldns_pkt_free(packet); 
    ldns_resolver_deep_free(resolver); 
}

void get_srv(const char *dns_config_path, const char *sip_man_log){
    ldns_resolver   *resolver; 
    ldns_pkt        *packet;
    ldns_rdf        *domain;
    ldns_rr_list    *response;
    ldns_status     status;
    uint16_t        prio = 0; 


    domain = ldns_dname_new_frm_str(naptr_replacement);
    status = ldns_resolver_new_frm_file(&resolver, dns_config_path); 
    packet = ldns_resolver_search(resolver, domain, LDNS_RR_TYPE_SRV, LDNS_RR_CLASS_IN, flags); 

    response = ldns_pkt_rr_list_by_type(packet, LDNS_RR_TYPE_SRV, LDNS_SECTION_ANSWER);
    for (size_t i = 0; i < ldns_rr_list_rr_count(response); i++) {
        ldns_rr *rr = ldns_rr_list_rr(response, i);
        prio = ldns_rdf2native_int16(ldns_rr_rdf(rr, 0));
        if (prio == 10) {
            strcpy(srv_prio10_target, ldns_rdf2str(ldns_rr_rdf(rr, 3)));
        } else if (prio == 20) {
            strcpy(srv_prio20_target, ldns_rdf2str(ldns_rr_rdf(rr, 3)));
        } else if (prio == 30) {
            strcpy(srv_prio30_target, ldns_rdf2str(ldns_rr_rdf(rr, 3)));
        } else {
            error_msg(sip_man_log, "(DNS) ERROR SRV: No valid entries found.");
        }
        srv_ttl = ldns_rr_ttl(rr);
    }

    ldns_rr_list_deep_free(response); 
    ldns_pkt_free(packet); 
    ldns_resolver_deep_free(resolver); 
}

void get_a_record(const char *dns_config_path, const char *target, char *globale_variable) {
    ldns_resolver   *resolver; 
    ldns_pkt        *packet;
    ldns_rdf        *domain, *rdata;
    ldns_rr_list    *response;
    ldns_status     status;


    domain = ldns_dname_new_frm_str(target);
    status = ldns_resolver_new_frm_file(&resolver, dns_config_path); 
    packet = ldns_resolver_search(resolver, domain, LDNS_RR_TYPE_A, LDNS_RR_CLASS_IN, flags); 

    response = ldns_pkt_rr_list_by_type(packet, LDNS_RR_TYPE_A, LDNS_SECTION_ANSWER);
    rdata = ldns_rr_rdf(ldns_rr_list_rr(response, 0), 0);
    if (rdata) {
        const uint8_t *addr = ldns_rdf_data(rdata); 
        snprintf(globale_variable, 64, "%d.%d.%d.%d", addr[0], addr[1], addr[2], addr[3]);
        
        a_record_ttl = ldns_rr_ttl(ldns_rr_list_rr(response, 0));
    }

    ldns_rr_list_deep_free(response); 
    ldns_pkt_free(packet); 
    ldns_resolver_deep_free(resolver); 
}

void resolve_dns(const char *dns_config_path, const char *domain, char *own_precedense, const char *sip_man_log){
    uint32_t            timer = 0;
    char                tmp[256];

    get_naptr(dns_config_path, domain, own_precedense);

    while (execute_loop) {
        if (srv_ttl <= 0) {
            error_msg(sip_man_log, "(DNS) INFO: SRV-TTL expired, performing new Request.");
            char tmp[64] = {0}; 
            if(srv_prio10_target[0] != '\0'){
                strcpy(tmp, srv_prio10_target);
            }
            get_srv(dns_config_path, sip_man_log);
            if (strncmp(tmp, srv_prio10_target, 64) != 0) {
                error_msg(sip_man_log, "(DNS) INFO: New Prio10-Destination announced in SRV, get new A-Record.");
                get_a_record(dns_config_path, srv_prio10_target, a_record_prio10);
                get_a_record(dns_config_path, srv_prio20_target, a_record_prio20);
                get_a_record(dns_config_path, srv_prio30_target, a_record_prio30);
            }
        }
        
        if (a_record_ttl <= 0) {
            error_msg(sip_man_log, "(DNS) INFO: A-Record-TTL expired, performing new Request.");
            get_a_record(dns_config_path, srv_prio10_target, a_record_prio10);
            get_a_record(dns_config_path, srv_prio20_target, a_record_prio20);
            get_a_record(dns_config_path, srv_prio30_target, a_record_prio30); 
        }

        if (srv_ttl > a_record_ttl) {
            timer = a_record_ttl;
        } else {
            timer = srv_ttl;
        }

        snprintf(tmp, sizeof(tmp), 
            "(DNS) INFO: \n"
            "\tCurrent SRV Prio10: %s\n"
            "\tCurrent A-Record Prio10: %s\n"
            "\tCurrent A-Record Prio20: %s\n"
            "\tCurrent A-Record Prio30: %s\n\n"
            "\tSRV-TTL: %d\n"
            "\tA-Record TTL: %d\n",
            srv_prio10_target, a_record_prio10, a_record_prio20, a_record_prio30, srv_ttl, a_record_ttl
        );
        error_msg(sip_man_log, tmp); 
        snprintf(tmp, sizeof(tmp), "(DNS) INFO: Sleep for: %d", timer);
        error_msg(sip_man_log, tmp);  
        sleep(timer);
        srv_ttl -= timer;
        a_record_ttl -= timer;
    }
    error_msg(sip_man_log, "(DNS) INFO: Cancelation-Request detected, Loop terminated");
}

void *start_dns_thread(void* args) {
    resolve_dns(dns_config_path, domain, own_precedense, sip_man_log);
}