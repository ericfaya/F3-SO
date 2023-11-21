#ifndef POOLE_LIST_H
#define POOLE_LIST_H
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>    // For write, getpid and fork functions

#define INET_ADDRSTRLEN 16

typedef struct PooleInfo {
    char userName[50];
    char ip[INET_ADDRSTRLEN];
    int port;
    int contador_bowmans;
} PooleInfo;

typedef struct PooleNode {
    PooleInfo info;
    struct PooleNode *next;
} PooleNode;

typedef struct {
    PooleNode *head;
} PooleList;

void init_poole_list(PooleList *list);
void add_poole(PooleList *list, PooleInfo pooleInfo);
void free_poole_list(PooleList *list);
PooleNode* searchPooleListLessBowmans(PooleList *list) ;
void removeBowmanFromPoole(PooleList *list,char *name);
#endif 