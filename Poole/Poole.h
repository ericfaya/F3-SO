#ifndef POOLE_H_
#define POOLE_H_
#define _GNU_SOURCE
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <stdint.h>
#include <stdio.h>     // For sprintf function
#include <unistd.h>    // For write, getpid and fork functions
#include <string.h>    // For strlen function
#include <stdlib.h>    // For exit function
#include <sys/wait.h>  // For wait function
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <pthread.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "Frame.h"


#define printF(x) write(1, x, strlen(x))
#define MAX_FDS 10

typedef struct
{
    char *fullName;
    char *pathName;
    char *ipDiscovery;
    int portDiscovery;
    char *ipPoole;
    int portPoole;
} Poole;

void listSongs(char *directory, char *result);
void sendSongListResponse(int socket);
int handleBowmanConnection(int *newsock,int errorSocketOrNot, Frame *incoming_frame) ;
void enviarAcknowledge(int newsock,int errorSocketOrNot);
void printaAcknowledge(char buffer[256], Frame *frame) ; //Nomes es per debugar
void freeAndClose(/*Frame *poole_frame,*/Poole *poolete,int numUsuaris);
void waitSocketBowman(int sockfd_bowman);
void connectToBowman(Poole *poolete);

#endif