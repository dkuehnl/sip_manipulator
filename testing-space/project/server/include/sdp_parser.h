#ifndef SDP_PARSER_H
#define SDP_PARSER_H

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
    psdp_i_t *media_title;
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
    psdp_list_t *global_session_attribute_list;
    psdp_list_t *media_list;
} psdp_t;

void parse_sdp_message(psdp_t *sdp, const char *sdp_message);
char *sdp_to_string(psdp_t *sdp);
void init_sdp(psdp_t **sdp);
void free_sdp(psdp_t *sdp);

#endif