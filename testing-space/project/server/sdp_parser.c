#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "./include/sdp_parser.h"

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
        } else if (strncmp(line, "c=", 2) == 0){
            m_line->connection_info = malloc(sizeof(psdp_c_t)); 
            m_line->connection_info->nettype = malloc(8);
            m_line->connection_info->addrtype = malloc(8); 
            m_line->connection_info->conn_addr = malloc(64);
            sscanf(line+2, "%s %s %s", m_line->connection_info->nettype, m_line->connection_info->addrtype, m_line->connection_info->conn_addr);
        } else if (strncmp(line, "i=", 2) == 0){
            m_line->media_title = malloc(sizeof(psdp_i_t));
            m_line->media_title->sess_desc = strdup(line+2); 
        } else if(strncmp(line, "b=", 2) == 0){
            psdp_b_t *b = malloc(sizeof(psdp_b_t));
            b->bw_type = malloc(64);
            b->bandwidth = malloc(64);
            sscanf(line+2, "%s %s", b->bw_type, b->bandwidth);
            psdp_list_element_t *element = create_list_element(b); 
            if(m_line->bandwidth_list->head == NULL){
                m_line->bandwidth_list->head = element; 
                m_line->bandwidth_list->tail = element; 
            } else {
                m_line->bandwidth_list->tail->next = element; 
                m_line->bandwidth_list->tail = element; 
            }
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

    free(copy); 
}

void parse_sdp_message(psdp_t *sdp, const char *sdp_message) {
    sdp->global_bandwidth_list = calloc(1, sizeof(psdp_list_t)); 
    sdp->media_list = calloc(1, sizeof(psdp_list_t)); 

    
    char *message_copy = strdup(sdp_message); 
    char *line = strtok(message_copy, "\r\n"); 
    while (line != NULL) {
        if(strcmp(line, "m=") != 0){
            parse_line(sdp, line);
        }
        line = strtok(NULL, "\r\n");
    }
    free(message_copy);
    message_copy = strdup(sdp_message);
    char *media = strstr(message_copy, "m=");
    if (media == NULL){
        printf("No m-line detected\n"); 
        free(message_copy); 
        return;
    }

    parse_media(sdp, media);
    
    free(message_copy);
}

char *sdp_to_string(psdp_t *sdp){
    size_t size = 1024;
    char *buffer = malloc(size); 
    
    if(sdp->version == NULL || sdp->origin == NULL || sdp->session_name == NULL || sdp->timing == NULL || sdp->media_list == NULL){
        printf("Error, mandatory-fields not available!\n");
        return NULL;
    } 
    snprintf(buffer, size, 
    "v=%s\r\n"
    "o=%s %s %s %s %s %s\r\n"
    "s=%s\r\n", 
    sdp->version->hvalue, 
    sdp->origin->name, sdp->origin->sess_id, sdp->origin->sess_v, sdp->origin->nettype, sdp->origin->addrtype, sdp->origin->unicast_ip, 
    sdp->session_name->sess_name);

    char tmp[64];
    if(sdp->session_info != NULL){
        snprintf(tmp, sizeof(tmp), "i=%s\r\n", sdp->session_info->sess_desc);
        strncat(buffer, tmp, size-strlen(buffer)); 
    }
    if(sdp->uri != NULL){
        snprintf(tmp, sizeof(tmp), "u=%s\r\n", sdp->uri->uri);
        strncat(buffer, tmp, size-strlen(buffer));
    }
    if(sdp->global_c_line != NULL){
        snprintf(tmp, sizeof(tmp), "c=%s %s %s\r\n", sdp->global_c_line->nettype, sdp->global_c_line->addrtype, sdp->global_c_line->conn_addr);
        strncat(buffer, tmp, size-strlen(buffer)); 
    }
    if(sdp->global_bandwidth_list != NULL){
        psdp_list_element_t *current_element = sdp->global_bandwidth_list->head; 
        while (current_element != NULL) {
            psdp_b_t *bw = (psdp_b_t*)current_element->element;
            snprintf(tmp, sizeof(tmp), "b=%s:%s\r\n", bw->bw_type, bw->bandwidth);
            strncat(buffer, tmp, size-strlen(buffer)); 
            current_element = current_element->next;
        }
    } 
    if(sdp->global_session_attribute_list != NULL){
        psdp_list_element_t *current_element = sdp->global_session_attribute_list->head;
        while (current_element != NULL){
            psdp_attribute_t *a = (psdp_attribute_t *)current_element->element;
            snprintf(tmp, sizeof(tmp), "a=%s\r\n", a->header);
            strncat(buffer, tmp, size-strlen(buffer)); 
            current_element = current_element->next;
        }
    }

    snprintf(tmp, sizeof(tmp), "t=%s %s\r\n", sdp->timing->start_time, sdp->timing->stop_time);
    strncat(buffer, tmp, size-strlen(buffer)); 

    if(sdp->repeat != NULL){
        snprintf(tmp, sizeof(tmp), "r=%s\r\n", sdp->repeat->r_value);
        strncat(buffer, tmp, size-strlen(buffer)); 
    }
    
    psdp_list_element_t *current_media = sdp->media_list->head;
    while(current_media != NULL){
        psdp_media_description_t *current_m_line = (psdp_media_description_t*) current_media->element;
        snprintf(tmp, sizeof(tmp), "m=%s %s %s %s\r\n", current_m_line->m_line->media, current_m_line->m_line->port, current_m_line->m_line->proto, current_m_line->m_line->fmt);
        strncat(buffer, tmp, size-strlen(buffer)); 

        if(current_m_line->media_title != NULL){
            snprintf(tmp, sizeof(tmp), "i=%s\r\n", current_m_line->media_title->sess_desc);
            strncat(buffer, tmp, size-strlen(buffer)); 
        } 
        if(current_m_line->connection_info != NULL){
            snprintf(tmp, sizeof(tmp), "c=%s %s %s\r\n", current_m_line->connection_info->nettype, current_m_line->connection_info->addrtype, current_m_line->connection_info->conn_addr);
            strncat(buffer, tmp, size-strlen(buffer));
        }
        if(current_m_line->bandwidth_list != NULL){
            psdp_list_element_t *b_media = current_m_line->bandwidth_list->head;
            while(b_media != NULL){
                psdp_b_t *b = (psdp_b_t *)b_media->element;
                snprintf(tmp, sizeof(tmp), "b=%s:%s\r\n", b->bw_type, b->bandwidth);
                strncat(buffer, tmp, size-strlen(buffer)); 
                b_media = b_media->next;
            }
        }
        psdp_list_element_t *attribute_list = current_m_line->attribute_list->head;
        while(attribute_list != NULL){
            psdp_attribute_t *current_a = (psdp_attribute_t*)attribute_list->element;
            if(current_a->value == NULL){
                snprintf(tmp, sizeof(tmp), "a=%s\r\n", current_a->header);
            } else {
                snprintf(tmp, sizeof(tmp), "a=%s:%s\r\n", current_a->header, current_a->value); 
            }
            strncat(buffer, tmp, size-strlen(buffer)); 
            attribute_list = attribute_list->next;
        }
        current_media = current_media->next;
    }
    
    return buffer; 
}

void init_sdp(psdp_t **sdp){
    *sdp = (psdp_t *)malloc(sizeof(psdp_t)); 
    if(*sdp != NULL){
        memset(*sdp, 0x00, sizeof(psdp_t)); 
    }
}

void free_sdp(psdp_t *sdp){
    if (!sdp) return;
    if (sdp->version){
        free(sdp->version->hvalue);
        free(sdp->version); 
    }
    if(sdp->uri){
        free(sdp->uri->uri);
        free(sdp->uri); 
    }
    if(sdp->timing){
        free(sdp->timing->start_time);
        free(sdp->timing->stop_time);
        free(sdp->timing); 
    }
    if(sdp->session_name){
        free(sdp->session_name->sess_name);
        free(sdp->session_name);
    }
    if(sdp->session_info){
        free(sdp->session_info->sess_desc); 
        free(sdp->session_info);
    }
    if(sdp->repeat){
        free(sdp->repeat->r_value);
        free(sdp->repeat);
    }
    if(sdp->origin){
        free(sdp->origin->addrtype);
        free(sdp->origin->name);
        free(sdp->origin->nettype);
        free(sdp->origin->sess_id);
        free(sdp->origin->sess_v);
        free(sdp->origin->unicast_ip);
        free(sdp->origin);
    }
    if(sdp->global_c_line){
        free(sdp->global_c_line->addrtype);
        free(sdp->global_c_line->conn_addr);
        free(sdp->global_c_line->nettype);
        free(sdp->global_c_line);
    }

    //Freigeben der Listen: 
    if(sdp->global_bandwidth_list){
        psdp_list_element_t *global_b_list = sdp->global_bandwidth_list->head;
        while (global_b_list != NULL){
            psdp_b_t *b = (psdp_b_t *)global_b_list->element;
            psdp_list_element_t *temp = global_b_list;
            free(b->bandwidth);
            free(b->bw_type);
            free(b);
            global_b_list = global_b_list->next;
            free(temp); 
        }
        free(sdp->global_bandwidth_list);
    }
    if(sdp->global_session_attribute_list){
        psdp_list_element_t *global_a_list = sdp->global_session_attribute_list->head;
        while(global_a_list != NULL){
            psdp_attribute_t *a = (psdp_attribute_t *)global_a_list->element;
            psdp_list_element_t *temp = global_a_list;
            free(a->header); 
            free(a->value); 
            free(a); 
            global_a_list = global_a_list->next;
            free(temp); 
        }
        free(sdp->global_session_attribute_list);
    }

    if(sdp->media_list){
        psdp_list_element_t *current_media = sdp->media_list->head;
        while(current_media != NULL){
            psdp_media_description_t *current_m_line = (psdp_media_description_t*) current_media->element;
            psdp_list_element_t *temp = current_media;
            if(current_m_line->media_title){
                free(current_m_line->media_title->sess_desc);
                free(current_m_line->media_title);
            }
            if(current_m_line->connection_info){
                free(current_m_line->connection_info->addrtype);
                free(current_m_line->connection_info->conn_addr);
                free(current_m_line->connection_info->nettype);
                free(current_m_line->connection_info);
            }
            if(current_m_line->m_line){
                free(current_m_line->m_line->fmt);
                free(current_m_line->m_line->media);
                free(current_m_line->m_line->port);
                free(current_m_line->m_line->proto);
                free(current_m_line->m_line);
            }
            
            if (current_m_line->bandwidth_list){
                psdp_list_element_t *media_b = current_m_line->bandwidth_list->head;
                while(media_b != NULL){
                    psdp_list_element_t *temp_b = media_b;
                    psdp_b_t *b = media_b->element;
                    free(b->bandwidth); 
                    free(b->bw_type);
                    free(b); 
                    media_b = media_b->next;
                    free(temp_b); 
                }
            }
            free(current_m_line->bandwidth_list);
            
            if(current_m_line->attribute_list){
                psdp_list_element_t *media_a = current_m_line->attribute_list->head;
                while(media_a != NULL){
                    psdp_list_element_t *temp_a = media_a;
                    psdp_attribute_t *a = (psdp_attribute_t *) media_a->element;
                    free(a->header);
                    free(a->value); 
                    free(a); 
                    media_a = media_a->next;
                    free(temp_a); 
                }
                free(current_m_line->attribute_list);
            }
            

            current_media = current_media->next;
            free(temp);
            free(current_m_line);
        }
        free(sdp->media_list);
    }
    free(sdp); 
}