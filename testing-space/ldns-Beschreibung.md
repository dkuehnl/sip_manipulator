ldns-Beschreibung: 

Query bauen, verschicken und Response speichern: 

domain = ldns_dname_new_frm_str("tel.t-online.de");     
    ->abzufragende Domain

status = ldns_resolver_new_frm_file(&resolver, NULL);   
    ->Resolver initialisieren

packet = ldns_resolver_search(resolver, domain, LDNS_RR_TYPE_NAPTR, LDNS_RR_CLASS_IN, NULL); 
    ->DNS-Quer losschicken, Art der Anfrage festlegen, Ergebnis in packet speichern 

response = ldns_pkt_rr_list_by_type(packet, LDNS_RR_TYPE_NAPTR, LDNS_SECTION_ANSWER);
    ->Ergebnis aus packet in entsprechende Structur formatieren und in response ablegen 


Absenden - NAPTR: 
packet = ldns_resolver_search(resolver, domain, LDNS_RR_TYPE_NAPTR, LDNS_RR_CLASS_IN, NULL); 
response = ldns_pkt_rr_list_by_type(packet, LDNS_RR_TYPE_NAPTR, LDNS_SECTION_ANSWER);

Auswerten - NAPTR: 
for (size_t i = 0; i < ldns_rr_list_rr_count(response); i++){
        ldns_rr *rr = ldns_rr_list_rr(response, i);                             ==> Anzahl der Responses durchgegen
        printf("Order: %u\n", ldns_rdf2native_int16(ldns_rr_rdf(rr, 0)));       ==> Prio
        printf("Preference: %u\n", ldns_rdf2native_int16(ldns_rr_rdf(rr, 1)));  
        printf("Flags: %s\n", ldns_rdf2str(ldns_rr_rdf(rr, 2)));            
        printf("Service: %s\n", ldns_rdf2str(ldns_rr_rdf(rr, 3)));              ==> Ob SIP UDP/SIP TCP/SIP TLS
        printf("RegEx: %s\n", ldns_rdf2str(ldns_rr_rdf(rr, 4)));
        printf("Replacement: %s\n", ldns_rdf2str(ldns_rr_rdf(rr, 5)));          ==> SRV-Query
        printf("----------------------------\n"); 
    }


Senden - SRV: 
packet = ldns_resolver_search(resolver, domain, LDNS_RR_TYPE_SRV, LDNS_RR_CLASS_IN, NULL); 
response = ldns_pkt_rr_list_by_type(packet, LDNS_RR_TYPE_SRV, LDNS_SECTION_ANSWER);

Auswerten - SRV:
for (size_t i = 0; i < ldns_rr_list_rr_count(response); i++) {
    ldns_rr *rr = ldns_rr_list_rr(response, i);                             ==> Anzahl der Responses durchgegen
    printf("Priority: %u\n", ldns_rdf2native_int16(ldns_rr_rdf(rr, 0)));    ==> Prio
    printf("Weight:: %u\n", ldns_rdf2native_int16(ldns_rr_rdf(rr, 1))); 
    printf("Port: %u\n", ldns_rdf2native_int16(ldns_rr_rdf(rr, 2)));        ==> Port (5060/5061)
    printf("Targe: %s\n", ldns_rdf2str(ldns_rr_rdf(rr, 3)));                ==> A-Record Query
    printf("----------------------\n");
}


Senden - A-Record: 
packet = ldns_resolver_search(resolver, domain, LDNS_RR_TYPE_A, LDNS_RR_CLASS_IN, NULL); 
response = ldns_pkt_rr_list_by_type(packet, LDNS_RR_TYPE_A, LDNS_SECTION_ANSWER);

Auswerten - A-Record: 
rdata = ldns_rr_rdf(ldns_rr_list_rr(response, 0), 0);                       ==> Ummappen in eine Struct (mit "response, 0" geht man die Responses durch)
    if (rdata) {
        const uint8_t *addr = ldns_rdf_data(rdata); 
        snprintf(ip_address, sizeof(ip_address), "%d.%d.%d.%d", addr[0], addr[1], addr[2], addr[3]); ==> Umsetzen in eine Variable 
        printf("IP-Adresse: %s\n", ip_address); 
    }

TTL-Allgemein abfrage: 
printf("TTL: %u\n", ldns_rr_ttl(rr));