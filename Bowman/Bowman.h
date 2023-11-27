#ifndef BOWMAN_H
#define BOWMAN_H
#define _GNU_SOURCE
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <stdio.h>     // For sprintf function
#include <unistd.h>    // For write, getpid and fork functions
#include <string.h>    // For strlen function
#include <stdlib.h>    // For exit function
#include <sys/wait.h>  // For wait function
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <time.h>
#include <ctype.h>
#include <limits.h>
#include <strings.h> 
#include "Frame.h"




#define printF(x) write(1, x, strlen(x))

typedef struct
{
    char *fullName;
    char *pathName;
    char *ipDiscovery;
    int portDiscovery;
} Bowman;


void freeMemory(Bowman* bowmaneta,int numUsuaris);
void printInfo(Bowman* bowmaneta);
void connectDiscovery(char *tokens[]);
void download(int *connectedOrNot, char *commandInput);
void listSongs(int *connectedOrNot);
void listPlaylists(int *connectedOrNot);
void checkDownload(int *connectedOrNot);
void clearDownload(int *connectedOrNot);
int controleCommands(char whichCommand[50],int *connectedOrNot);
void kctrlc();
void logout();
int connectBowman(char *tokens[]);

#endif