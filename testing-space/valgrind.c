#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct {
    int msg_nr;
    char *value;
} psdp_type_t;

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
    char *message;
    psdp_type_t *type;
    psdp_list_t *liste;
} psdp_t;

psdp_list_element_t *create_list_element(void *element) {
    psdp_list_element_t *new_element = malloc(sizeof(psdp_list_element_t));
    new_element->element = element; 
    new_element->next = NULL;

    return new_element;
}


void init_sdp(psdp_t **sdp){
    *sdp = malloc(sizeof(psdp_t)); 
    if (*sdp != NULL){
        memset(*sdp, 0x00, sizeof(psdp_t));
    }
}

void free_sdp(psdp_t *sdp){
    free(sdp->type->value);
    free(sdp->type);
    free(sdp->message);

    if(sdp->liste){
        psdp_list_element_t *global_b_list = sdp->liste->head;
        while (global_b_list != NULL){
            psdp_type_t *b = (psdp_type_t *)global_b_list->element;
            psdp_list_element_t *temp = global_b_list;
            free(b->value);
            free(b);
            global_b_list = global_b_list->next;
            free(temp); 
        }
        free(sdp->liste);
    }
    free(sdp); 
}

void fill_struct(psdp_t *sdp) {
    char tmp[] = "Test";
    sdp->liste = calloc(1, sizeof(psdp_list_t));
    sdp->number = 1;
    sdp->message = strdup("Hello"); 
    sdp->type = malloc(sizeof(psdp_type_t));
    sdp->type->msg_nr = 2;
    sdp->type->value = malloc(12); 
    sscanf(tmp, "%s", sdp->type->value);

    psdp_type_t *t = malloc(sizeof(psdp_type_t));
    t->msg_nr = 3;
    t->value = malloc(12); 
    sscanf(tmp, "%s", t->value);
    psdp_list_element_t *element = create_list_element(t);
    if (sdp->liste->head == NULL){
        sdp->liste->head = element; 
        sdp->liste->tail = element; 
    } else {
        sdp->liste->tail->next = element; 
        sdp->liste->tail = element; 
    }
}

int main(){
    psdp_t *sdp_struct = NULL; 

    init_sdp(&sdp_struct); 
    if (sdp_struct->message == NULL){
        printf("Fehler aufgetreten\n"); 
    }
    fill_struct(sdp_struct);
    printf("Number: %d\n", sdp_struct->number);
    printf("Mes: %s\n", sdp_struct->message);
    printf("Type-Nr.: %d\nType-Msg: %s\n", sdp_struct->type->msg_nr, sdp_struct->type->value);

    psdp_list_element_t *liste = sdp_struct->liste->head;
    while (liste != NULL) {
        psdp_type_t *type = (psdp_type_t*)liste->element;
        printf("Type-Value: %s\n", type->value);
        liste = liste->next;
    }

    free_sdp(sdp_struct); 

    return 0; 
}