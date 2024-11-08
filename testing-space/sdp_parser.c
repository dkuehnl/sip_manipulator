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
    char *re_int;
    char *act_dur;
    char *offset;
} psdp_r_t;

typedef struct {
    char *media;
    char *port; 
    char *proto;
    char *fmt;
} psdp_m_t;

typedef struct {
    char *attribute;
    char *avalue;
} psdp_avalues_t;

typedef struct {
    char *attribue;
} psdp_aonly_t;

typedef struct psdp_list_element {
    void *element;
    struct psdp_list_element *next;
} psdp_list_element_t;

typedef struct {
    psdp_list_element_t *head;
    psdp_list_element_t *tail;
} psdp_list_t;

typedef struct {
    psdp_v_t *version;
    psdp_o_t *origin;
    psdp_s_t *session_name;
    psdp_i_t *session_info;
    psdp_u_t *uri;
    psdp_list_t *connection_list;
    psdp_list_t *bandwidth_list;
    psdp_t_t *timing;
    psdp_r_t *repeat;
    psdp_list_t *media_list;
    psdp_list_t *attribute_list;
} psdp_t;

void psdp_list_add(psdp_list_t *list, void *element) {
    psdp_list_element_t *new_element = malloc(sizeof(psdp_list_element_t));
    if (!new_element) return; 

    new_element->element = element; 
    new_element->next = NULL;

    if (list->tail) {
        list->tail->next = new_element; 
    } else {
        list->head = new_element;
    }
    list->tail = new_element; 
}

void process_list(psdp_list_t *list) {
    psdp_list_element_t *current = list->head;

    while (current){

    }
}

int main() {
    char sdp[2048];

    snprintf(sdp, sizeof(sdp), 
    "v=0\r\n"
    "o=alice 2890844526 2890842807 IN IP4 pc33.atlanta.com\r\n"
    "s=Session SDP\r\n"
    "c=IN IP4 192.0.2.1\r\n"
    "t=0 0\r\n"
    "m=audio 49170 RTP/AVP 0 8 118 119\r\n"
    "a=rtpmap:0 PCMU/8000\r\n"
    "a=rtpmap:8 PCMA/8000\r\n"
    "a=rtpmap:118 telephone-event/8000\r\n"
    "a=fmtp: 0-15\r\n"
    "a=rtpmap:119 telephone-event/16000\r\n"
    "a=fmtp:0 annexb=no\r\n"
    "a=ptime:20\r\n"
    "a=maxptime:150\r\n"
    "a=sendrecv\r\n"
    );

    psdp_t *parsed_sdp;
    char *start = strstr(sdp, "o=");
    printf("%s", start); 

    return 0;
}