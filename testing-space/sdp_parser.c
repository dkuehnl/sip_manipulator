#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct {
    char *hvalue;
} psdp_v_t;

typedef struct {
    char *name; 
    char *sess_id; 
    char *sess_v; 
    char *nettype; 
    char *addrtype; 
    char *unicast_ip;
} psdp_o_t;

typedef struct {
    char *sess_name;
} psdp_s_t;

typedef struct {
    char *sess_desc;
} psdp_i_t;

typedef struct { 
    char *uri;
} psdp_u_t;

typedef struct {
    char *nettype;
    char *addrtype; 
    char *conn_addr;
} psdp_c_t;

typedef struct {
    char *bw_type;
    char *bandwidth;
} psdp_b_t;

typedef struct {
    char *start_time;
    char *stop_time;
} psdp_t_t;

typedef struct {
    char *r_value;
} psdp_r_t;

typedef struct {
    char *media;
    char *port; 
    char *proto;
    char *fmt;
} psdp_m_t;

typedef struct {
    int order;
    char *header;
    char *value;
} psdp_attribute_t;

typedef struct psdp_list_element {
    void *element;
    struct psdp_list_element *next;
} psdp_list_element_t;

typedef struct {
    psdp_list_element_t *head;
    psdp_list_element_t *tail;
} psdp_list_t;

typedef struct {
    int number;
    psdp_m_t *m_line;
    psdp_c_t *connection_info; 
    psdp_list_t *bandwidth_list; 
    psdp_list_t *attribute_list;
} psdp_media_description_t;

typedef struct {
    psdp_v_t *version;
    psdp_o_t *origin;
    psdp_s_t *session_name;
    psdp_i_t *session_info;
    psdp_u_t *uri;
    psdp_t_t *timing;
    psdp_r_t *repeat;
    psdp_c_t *global_c_line;
    psdp_list_t *global_bandwidth_list;
    psdp_list_t *media_list;
} psdp_t;

psdp_list_element_t *create_list_element(void *element) {
    psdp_list_element_t *new_element = malloc(sizeof(psdp_list_element_t));
    new_element->element = element; 
    new_element->next = NULL;

    return new_element;
}

void parse_line(psdp_t *sdp, const char *line) {
    char header[3];
    strncpy(header, line, 2); 
    header[2] = '\0';

    const char *value = line + 2;
    if (strcmp(header, "v=") == 0){
        sdp->version = malloc(sizeof(psdp_v_t)); 
        sdp->version->hvalue = strdup(value);
    } else if (strcmp(header, "o=") == 0){
        sdp->origin = malloc(sizeof(psdp_o_t));
        sdp->origin->name = malloc(64); 
        sdp->origin->sess_id = malloc(128);
        sdp->origin->sess_v = malloc(128); 
        sdp->origin->nettype = malloc(16);
        sdp->origin->addrtype = malloc(16);
        sdp->origin->unicast_ip = malloc(40);
        sscanf(value, "%s %s %s %s %s %s", sdp->origin->name, sdp->origin->sess_id, sdp->origin->sess_v, sdp->origin->nettype, sdp->origin->addrtype, sdp->origin->unicast_ip );
    } else if (strcmp(header, "s=") == 0){
        sdp->session_name = malloc(sizeof(psdp_s_t));
        sdp->session_name->sess_name = strdup(value);
    } else if (strcmp(header, "i=") == 0){
        sdp->session_info = malloc(sizeof(psdp_i_t));
        sdp->session_info->sess_desc = strdup(value); 
    } else if (strcmp(header, "u=") == 0){
        sdp->uri = malloc(sizeof(psdp_u_t));
        sdp->uri->uri = strdup(value);
    } else if (strcmp(header, "c=") == 0){
        sdp->global_c_line = malloc(sizeof(psdp_c_t)); 
        sdp->global_c_line->nettype = malloc(16);
        sdp->global_c_line->addrtype = malloc(16);
        sdp->global_c_line->conn_addr = malloc(40);
        sscanf(value, "%s %s %s", sdp->global_c_line->nettype, sdp->global_c_line->addrtype, sdp->global_c_line->conn_addr);
    } else if (strcmp(header, "b=") == 0) {
        psdp_b_t *b_line = malloc(sizeof(psdp_b_t));
        b_line->bandwidth = malloc(64);
        b_line->bw_type = malloc(64);
        sscanf(value, "%s %s", b_line->bw_type, b_line->bandwidth);
        psdp_list_element_t *element = create_list_element(b_line); 
        if(sdp->global_bandwidth_list->head == NULL){
            sdp->global_bandwidth_list->head = element; 
            sdp->global_bandwidth_list->tail = element; 
        } else {
            sdp->global_bandwidth_list->tail->next = element; 
            sdp->global_bandwidth_list->tail = element; 
        }
    } else if (strcmp(header, "t=") == 0){
        sdp->timing = malloc(sizeof(psdp_t_t)); 
        sdp->timing->start_time = malloc(64);
        sdp->timing->stop_time = malloc(64);
        sscanf(value, "%s %s", sdp->timing->start_time, sdp->timing->stop_time);
    } else if(strcmp(header, "r=") == 0){
        sdp->repeat = malloc(sizeof(psdp_r_t));
        sdp->repeat->r_value = strdup(value); 
    }
}

void parse_media(psdp_t *sdp, const char *media){
    char *copy = strdup(media);
    char *line = strtok(copy, "\r\n"); 
    int m_count = 0;  
    int a_order = 0; 
    psdp_media_description_t *m_line = malloc(sizeof(psdp_media_description_t)); 
    m_line->attribute_list = calloc(1, sizeof(psdp_list_element_t));
    m_line->number = m_count; 
    while (line != NULL){
        if(strncmp(line, "m=", 2) == 0 && m_count == 0){
            //printf("%s\n", line); 
            m_line->m_line = malloc(sizeof(psdp_m_t)); 
            m_line->m_line->media = malloc(16); 
            m_line->m_line->port = malloc(24); 
            m_line->m_line->proto = malloc(32); 
            m_line->m_line->fmt = malloc(40); 
            sscanf(line+2, "%s %s %s %[^\n]", m_line->m_line->media, m_line->m_line->port, m_line->m_line->proto, m_line->m_line->fmt);
            m_count++;
        } else if (strncmp(line, "m=", 2) == 0 && m_count > 0){
            printf("Multiple m-line detected, not implemented yet\n"); 
            return;
        } else if(strncmp(line, "a=", 2) == 0){
            psdp_attribute_t *a = malloc(sizeof(psdp_attribute_t)); 
            a->order = a_order++;
            char *col = strchr(line+2, ':'); 
            if(col != NULL){
                a->value = strdup(col+1); 
                *col = '\0';
                a->header = strdup(line+2); 
            } else {
                a->header = strdup(line+2); 
                a->value = NULL; 
            }
            psdp_list_element_t *element = create_list_element(a); 
            if (m_line->attribute_list->head == NULL){
                m_line->attribute_list->head = element; 
                m_line->attribute_list->tail = element; 
            }else {
                m_line->attribute_list->tail->next = element; 
                m_line->attribute_list->tail = element; 
            }
        }

        line = strtok(NULL, "\r\n"); 
    }
    
    psdp_list_element_t *element = create_list_element(m_line); 
    if (sdp->media_list->head == NULL) {
        sdp->media_list->head = element; 
        sdp->media_list->tail = element; 
    } else {
        sdp->media_list->tail->next = element; 
        sdp->media_list->tail = element; 
    }
}

psdp_t *parse_sdp_message(const char *sdp_message) {
    psdp_t *sdp = calloc(1, sizeof(psdp_t)); 
    sdp->global_bandwidth_list = calloc(1, sizeof(psdp_list_t)); 
    sdp->media_list = calloc(1, sizeof(psdp_list_t)); 

    
    char *message_copy = strdup(sdp_message); 
    char *line = strtok(message_copy, "\r\n"); 
    //int test = 0; 
    while (line != NULL) {
        if(strcmp(line, "m=") != 0){
            parse_line(sdp, line);
        }
        line = strtok(NULL, "\r\n");
    }
    message_copy = strdup(sdp_message);
    char *media = strstr(message_copy, "m=");
    if (media == NULL){
        printf("No m-line detected\n"); 
        free(message_copy); 
        return NULL; 
    }

    parse_media(sdp, media); 
    
    free(message_copy);
    return sdp;
}

char *sdp_to_string(psdp_t *sdp){
    size_t size = 1024;
    char *buffer = malloc(size); 

    snprintf(buffer, size, "v=%s\r\n", sdp->version->hvalue);
    return buffer; 
}

void init_sdp(psdp_t **sdp){
    *sdp = (psdp_t *)malloc(sizeof(psdp_t)); 
    if(*sdp != NULL){
        memset(sdp, 0x00, sizeof(psdp_t)); 
    }
}

int main() {
    char sdp[2048];

    snprintf(sdp, sizeof(sdp), 
    "v=0\r\n"
    "o=alice 2890844526 2890842807 IN IP4 pc33.atlanta.com\r\n"
    "s=Session SDP\r\n"
    "c=IN IP4 192.0.2.1\r\n"
    "b=AS 90\r\n"
    "b=AS 120\r\n"
    "b=CT 10000\r\n"
    "t=1 2\r\n"
    "r=604800 3600 0 9000\r\n"
    "m=audio 49170 RTP/AVP 0 8 118 119\r\n"
    "a=rtpmap:0 PCMU/8000\r\n"
    "a=rtpmap:8 PCMA/8000\r\n"
    "a=rtpmap:118 telephone-event/8000\r\n"
    "a=fmtp:118 0-15\r\n"
    "a=rtpmap:119 telephone-event/16000\r\n"
    "a=fmtp:119 annexb=no\r\n"
    "a=ptime:20\r\n"
    "a=maxptime:150\r\n"
    "a=sendrecv\r\n"
    );

    psdp_t *parsed_sdp;
    init_sdp(&parsed_sdp);
    if (parsed_sdp->version != NULL){
        printf("erfolgreich!\n");
        return 0;
    }
    
    printf("nicht erfolgreich\n");
    return 0;
    parse_sdp_message(sdp);
    char *msg = sdp_to_string(parsed_sdp); 
    printf("%s", msg); 
    /*
    psdp_list_element_t *current_media = parsed_sdp->media_list->head;
    while(current_media != NULL){
        psdp_media_description_t *current_m_line = (psdp_media_description_t*) current_media->element;
        psdp_list_element_t *attribute_list = current_m_line->attribute_list->head;
        while(attribute_list != NULL){
            psdp_attribute_t *current_a = (psdp_attribute_t*)attribute_list->element;
            printf("Nr.: %d\nHeader: %s\nValue: %s\n", current_a->order, current_a->header, current_a->value); 
            attribute_list = attribute_list->next;
        }
        current_media = current_media->next;
    }*/
    /*psdp_list_element_t *current_m = parsed_sdp->media_list->head; 
    while(current_m != NULL){
        psdp_media_description_t *a = (psdp_media_description_t*)current_m->element; 
        printf("Media: %s\nPort: %s\nProto: %s\nCodec: %s\n", a->m_line->media, a->m_line->port, a->m_line->proto, a->m_line->fmt);
        current_m = current_m->next;
    }*/
    /*printf("Version: %s\n", parsed_sdp->version->hvalue); 
    printf("O-Line:\n%s\n%s\n%s\n%s\n%s\n%s\n", parsed_sdp->origin->name, parsed_sdp->origin->sess_id, parsed_sdp->origin->sess_v, parsed_sdp->origin->nettype, parsed_sdp->origin->addrtype, parsed_sdp->origin->unicast_ip);
    printf("C-Line:\n%s\n%s\n%s\n", parsed_sdp->global_c_line->nettype, parsed_sdp->global_c_line->addrtype, parsed_sdp->global_c_line->conn_addr); 
    psdp_list_element_t *current = parsed_sdp->global_bandwidth_list->head; 
    while (current != NULL) {
        psdp_b_t *bw = (psdp_b_t*)current->element;
        printf("B: %s, %s\n", bw->bw_type, bw->bandwidth);
        current = current->next;
    }
    printf("timing: %s, %s\n", parsed_sdp->timing->start_time, parsed_sdp->timing->stop_time);
    printf("repeat: %s\n", parsed_sdp->repeat->r_value); 
    psdp_list_element_t *current_m = parsed_sdp->media_list->head;
    while(current_m != NULL){
        psdp_media_description_t *m = (psdp_media_description_t *)current_m->element;
        printf("Num: %d:\nMedia: %s\nPort: %s\nProtokoll: %s\nCodecs: %s\n", m->number, m->m_line->media, m->m_line->port, m->m_line->proto, m->m_line->fmt);
        current_m = current_m->next;
    }*/

    /*printf("ptime: %s\n", parsed_sdp->ptime->attribue); 
    psdp_list_element_t *current_a = parsed_sdp->rtpmap_list->head;
    while (current_a != NULL) {
        psdp_rtpmap_t *a = (psdp_rtpmap_t *)current_a->element;
        printf("rtpmap: %s - %s\n", a->codec, a->value);
        current_a = current_a->next;
    }
    psdp_list_element_t *current_f = parsed_sdp->fmtp_list->head; 
    while (current_f != NULL){
        psdp_fmtp_t *f = (psdp_fmtp_t *)current_f->element;
        printf("fmtp: %s - %s\n", f->codec, f->value);
        current_f = current_f->next;
    }
    psdp_list_element_t *current_a_only = parsed_sdp->attribute_list->head;
    while(current_a_only != NULL) {
        psdp_aonly_t *a = (psdp_aonly_t *)current_a_only->element;
        printf("a: %s\n", a->attribue); 
        current_a_only = current_a_only->next;
    }*/
    return 0;
}


//Neo4J: frAmAqiAfe0zHjdisw_cxsyNESfxUojMe4WjH5vVVyw