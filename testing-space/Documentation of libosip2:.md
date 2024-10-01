Documentation of libosip2: 

SIP-Message Parsen: 

    1. osip_message_t *sip -> Deklarieren einer Message-Variable
    2. osip_message_init(&sip) -> Initialisieren der Variable 
    3. osip_message_parse(sip, buffer, strlen(buffer)) -> Eigentliche Parsen (Buffer ist die SIP-Nachricht vom Socket)

Header erkennen, bearbeiten und auslesen: 
Known-Header: from, to, call-id, contact 

Beispiel: From

    1. osip_from_t *from = osip_message_get_from(sip) -> From-Header aus geparster SIP in from-struct speichern 
        1.1: Header ausgeben: osip_from_to_str(from, &test) -> &test (char*) kann dann in printf ausgegeben werden
    2. char *output = osip_from_get_displayname(from) -> füllt *output mit Displayname (falls vorhanden, wenn nicht mit NULL)
    3. osip_uri_t *uri = osip_uri_get_url(from) -> extrahieren des restlichen Inhalts des From-Headers in die uri-struct 
    3. char *output =   osip_uri_get_scheme(uri)    -> sip/sips
                        ospi_uri_get_host(uri)      -> Host-Part (ohne  @)
                        osip_uri_get_username(uri)  -> User-Part (zwischen sip: und @)
    
    Oder kurz: sip->from->url->username

### Liste der Implementierten Header:
1. accept
2. alert-info
3. allow
4. authorization
5. call-id
6. contact
7. content-length /-type
8. cseq
9. from/to
10. record-route
11. route
12. via

### Implementierte Header in deren Werte abfragen: 

```
if (strncmp(entries.headers[i], "From", 4) == 0) {
            printf("From-Header:\n");
            printf("Scheme:\t\t%s\n", sip->from->url->scheme);
            printf("User-Part:\t%s\n", sip->from->url->username);
            printf("Host-Part:\t%s\n", sip->from->url->host);
        } else if (strncmp(entries.headers[i], "To", 2) == 0) {
            printf("To-Header:\n");
            printf("Scheme:\t\t%s\n", sip->to->url->scheme);
            printf("User-Part:\t%s\n", sip->to->url->username);
            printf("Host-Part:\t%s\n", sip->to->url->host);
        } else if (strncmp(entries.headers[i], "Call-ID", 7) == 0) {
            printf("Call-ID-Header:\n");
            printf("Host:\t\t%s\n", sip->call_id->host);
            printf("Number:\t%s\n", sip->call_id->number);
        } else if (strncmp(entries.headers[i], "Contact", 7) == 0) {
            osip_contact_t *contact;
            osip_message_get_contact(sip, 0, &contact);
            printf("Contact-Header:\n");
            printf("Host:\t\t%s\n", contact->url->host); (IP-Adresse)
            printf("Passwort:\t\t%s\n", contact->url->password); (NULL)
            printf("Port:\t\t%s\n", contact->url->port); (Port)
            printf("Scheme:\t\t%s\n", contact->url->scheme); (sip/sips)
            printf("String:\t\t%s\n", contact->url->string); (NULL)
            printf("User:\t\t%s\n", contact->url->username); (Pilotnummer)
            
            osip_uri_t *uri = contact->url;
            for (int j = 0; j < osip_list_size(&uri->url_params); j++) {
                osip_uri_param_t *param = osip_list_get(&uri->url_params, j);
                printf("Type: %s\nValue: %s\n", param->gname, param->gvalue);
            }
        } else if (strncmp(entries.headers[i], "CSeq", 7) == 0) {
            printf("CSeq-Header:\n");
            printf("Number:\t\t%s\n", sip->cseq->number);
            printf("Method:\t\t%s\n", sip->cseq->method);
        } else if (strncmp(entries.headers[i], "Via", 7) == 0) {
            printf("VIA-Header:\n");

            for (int x = 0; x < osip_list_size(&sip->vias); x++) {
                osip_via_t *via = osip_list_get(&sip->vias, x);

                printf("Version:\t%s\n", via->version);
                printf("Protocol:\t%s\n", via->protocol);
                printf("Host:\t\t%s\n", via->host);
                printf("Port:\t\t%s\n", via->port);
                printf("Comment:\t%s\n", via->comment);
            }    
        } 
    }```
