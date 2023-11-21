#include "PooleList.h"

void init_poole_list(PooleList *list) {
    list->head = NULL;
}

void add_poole(PooleList *list, PooleInfo pooleInfo) {
    PooleNode *newNode = (PooleNode *)malloc(sizeof(PooleNode));
    
    if (!newNode) {
        perror("Unable to allocate memory for new Poole");
        exit(EXIT_FAILURE);
    }
    newNode->info = pooleInfo;
    newNode->next = list->head;
    list->head = newNode;
}

void free_poole_list(PooleList *list) {
    PooleNode *current = list->head;
    while (current != NULL) {
        PooleNode *next = current->next;
        free(current);
        current = next;
    }
    list->head = NULL;
}
           
PooleNode* searchPooleListLessBowmans(PooleList *list) {
    PooleNode *current = list->head;
    PooleNode *resultNode=NULL;
    int bowmansMaxim=50;
    while (current != NULL) {//Si trobem un poole que tingui menys bowmans establim el valor 
        if(current->info.contador_bowmans<bowmansMaxim){ //maxim a aquell poole i nem a bbuscar si 
            bowmansMaxim=current->info.contador_bowmans;  //hi ha algun poole mes que tingui menys que el que acabem de trobar
            resultNode=current;
        }
        current = current->next;
    }
    //current->info.contador_bowmans++;
    return resultNode;
}

void removeBowmanFromPoole(PooleList *list,char *name){
    PooleNode *current = list->head;
    while (current != NULL) {
        if(strcmp(current->info.userName,name)==0){
            current->info.contador_bowmans--;
        }
        current = current->next;
    }
}