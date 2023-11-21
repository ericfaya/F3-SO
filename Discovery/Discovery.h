#ifndef DISCOVERY_H
#define DISCOVERY_H
#define _GNU_SOURCE
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "PooleList.h"
#include "Frame.h"

#define printF(x) write(1, x, strlen(x))

typedef struct
{
    char *ipPoole;
    int portPoole;
    char *ipBowman;
    int portBowman;
} Discovery;

void process_frame(Frame *frame, PooleList *list);
void freeAndClose(PooleList *pooleList,int sockfd_poole,int sockfd_bowman);
void waitSocketPoole(int sockfd_poole,PooleList *pooleList);
void waitSocketBowman(int sockfd_bowman,PooleList *pooleList);
void enviarAcknowledge(int newsock,int errorSocketOrNot,int bowmanOrPoole,PooleList *pooleList);
void process_frame(Frame *frame, PooleList *list);

#endif