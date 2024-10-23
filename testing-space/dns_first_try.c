#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <ldns/ldns.h>

char        naptr_replacement[128] = {0};

uint32_t    srv_ttl = 0; 
char        prio10_target[64] = {0};
char        prio20_target[64] = {0};
char        prio30_target[64] = {0};

uint32_t    a_record_ttl = 0;
char        a_record_prio10[64] = {0};
char        a_record_prio20[64] = {0};
char        a_record_prio30[64] = {0};


void get_naptr(const char *dns_config_path, const char *dns_name, const char *precedense){
    ldns_resolver   *resolver; 
    ldns_pkt        *packet;
    ldns_rdf        *domain;
    ldns_rr_list    *response;
    ldns_status     status;

    char naptr_service[32] = {0};


    domain = ldns_dname_new_frm_str(dns_name);
    status = ldns_resolver_new_frm_file(&resolver, dns_config_path); 
    packet = ldns_resolver_search(resolver, domain, LDNS_RR_TYPE_NAPTR, LDNS_RR_CLASS_IN, NULL); 

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

void get_srv(const char *dns_config_path){
    ldns_resolver   *resolver; 
    ldns_pkt        *packet;
    ldns_rdf        *domain;
    ldns_rr_list    *response;
    ldns_status     status;

    uint16_t prio = 0; 
    domain = ldns_dname_new_frm_str(naptr_replacement);
    status = ldns_resolver_new_frm_file(&resolver, dns_config_path); 
    packet = ldns_resolver_search(resolver, domain, LDNS_RR_TYPE_SRV, LDNS_RR_CLASS_IN, NULL); 

    response = ldns_pkt_rr_list_by_type(packet, LDNS_RR_TYPE_SRV, LDNS_SECTION_ANSWER);
    for (size_t i = 0; i < ldns_rr_list_rr_count(response); i++) {
        ldns_rr *rr = ldns_rr_list_rr(response, i);
        prio = ldns_rdf2native_int16(ldns_rr_rdf(rr, 0));
        if (prio == 10) {
            strcpy(prio10_target, ldns_rdf2str(ldns_rr_rdf(rr, 3)));
        } else if (prio == 20) {
            strcpy(prio20_target, ldns_rdf2str(ldns_rr_rdf(rr, 3)));
        } else if (prio == 30) {
            strcpy(prio30_target, ldns_rdf2str(ldns_rr_rdf(rr, 3)));
        } else {
            printf("Problem");
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
    packet = ldns_resolver_search(resolver, domain, LDNS_RR_TYPE_A, LDNS_RR_CLASS_IN, NULL); 

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

int main(){
    char *config = "./resolv.conf";
    char *domain = "555111584764.primary.companyflex.de";
    char *own_precedense = "\"SIP+D2T\""; 
    uint32_t timer = 0; 
    get_naptr(config, domain, own_precedense);   

    while (1) {
        if (srv_ttl <= 0) {
            printf("SRV-TTL expired, performing new Request.\n"); 
            char tmp[64] = {0}; 
            if(prio10_target[0] != '\0'){
                strcpy(tmp, prio10_target);
                printf("TMP: %s\n", tmp); 
                printf("Prio: %s\n", prio10_target);
            }
            get_srv(config); 
            if (strncmp(tmp, prio10_target, 64) != 0) {
                printf("New Prio10-Destination announced\n"); 
                get_a_record(config, prio10_target, a_record_prio10);
                printf("%s\n", a_record_prio10);
                get_a_record(config, prio20_target, a_record_prio20);
                get_a_record(config, prio30_target, a_record_prio30); 
            }
        }
        
        if (a_record_ttl <= 0) {
            printf("A-Record-TTL expired, performing new Request.\n");
            get_a_record(config, prio10_target, a_record_prio10);
            get_a_record(config, prio20_target, a_record_prio20);
            get_a_record(config, prio30_target, a_record_prio30); 
        }

        if (srv_ttl > a_record_ttl) {
            timer = a_record_ttl;
        } else {
            timer = srv_ttl;
        }

        printf("SRV Prio10: %s\n", prio10_target); 
        printf("SRV TTL: %d\n", srv_ttl); 
        printf("A-Record Result 1: %s\n", a_record_prio10); 
        printf("A-Record Result 2: %s\n", a_record_prio20); 
        printf("A-Record Result 3: %s\n", a_record_prio30); 
        printf("A-Record TTL: %d\n\n", a_record_ttl);
        
        printf("Sleep for: %d\n", timer); 
        sleep(20);
        srv_ttl -= timer;
        a_record_ttl -= timer;
        printf("DEBUG: new TTL-Values for testing.\nSRV-TTL: %d\nA-TTL: %d\n", srv_ttl, a_record_ttl);
    }

    return 0; 
}

    /*rdata = ldns_rr_rdf(ldns_rr_list_rr(response, 2), 0);
    if (rdata) {
        const uint8_t *addr = ldns_rdf_data(rdata); 
        snprintf(ip_address, sizeof(ip_address), "%d.%d.%d.%d", addr[0], addr[1], addr[2], addr[3]);
        printf("IP-Adresse: %s\n", ip_address); 
    }*/

       /*for (size_t i = 0; i < ldns_rr_list_rr_count(response); i++){
        ldns_rr *rr = ldns_rr_list_rr(response, i); 
        printf("Order: %u\n", ldns_rdf2native_int16(ldns_rr_rdf(rr, 0)));
        printf("Preference: %u\n", ldns_rdf2native_int16(ldns_rr_rdf(rr, 1)));
        printf("Flags: %s\n", ldns_rdf2str(ldns_rr_rdf(rr, 2)));
        printf("Service: %s\n", ldns_rdf2str(ldns_rr_rdf(rr, 3)));
        printf("RegEx: %s\n", ldns_rdf2str(ldns_rr_rdf(rr, 4)));
        printf("Replacement: %s\n", ldns_rdf2str(ldns_rr_rdf(rr, 5)));
        printf("----------------------------\n"); 
    }*/

       /*for (size_t i = 0; i < ldns_rr_list_rr_count(response); i++) {
        ldns_rr *rr = ldns_rr_list_rr(response, i);
        printf("Priority: %u\n", ldns_rdf2native_int16(ldns_rr_rdf(rr, 0))); 
        printf("Weight:: %u\n", ldns_rdf2native_int16(ldns_rr_rdf(rr, 1))); 
        printf("Port: %u\n", ldns_rdf2native_int16(ldns_rr_rdf(rr, 2))); 
        printf("Targe: %s\n", ldns_rdf2str(ldns_rr_rdf(rr, 3)));
        printf("----------------------\n");
    }*/