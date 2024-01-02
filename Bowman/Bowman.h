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
#include <sys/ipc.h>
#include <sys/msg.h>


#include "frame.h"
#include "md5functions.h"
#include "semaphore_v2.h"

#define printF(x) write(1, x, strlen(x))

typedef struct
{
    char *fullName;
    char *pathName;
    char *ipDiscovery;
    int portDiscovery;
} Bowman;

typedef struct {
    char *fileName;
    int fileSize;
    char *md5sum;
    char *songPath;
    int songId;
    int fd_song;
    ssize_t totalBytesReceived;
    int id_queue;
    int id_bustia;
    int id_bustiaToCheck;
    //int incrementBustiaToCheckDownload=id_queue+1000;
} FileInfo;

typedef struct {
    long mtype;
    Frame frame;
}MessageQueue;

typedef struct {
    int mq_id;
    int newCommand;
    //MessageQueue *msg;
} ThreadArgs;

typedef struct SongNode {
    FileInfo *fileInfo;//Ho fem constant ja que no voldrem modificar res,nomes veure informacio
    struct SongNode* next;
} SongNode;

void freeMemory();
void printInfo(Bowman* bowmaneta);
void connectDiscovery(char *tokens[]);
void download(char *commandInput) ;
void listSongs();
void listPlaylists();
void checkDownload(int *connectedOrNot);
void clearDownload(int *connectedOrNot);
int controleCommands(char whichCommand[50],int *connectedOrNot);
void kctrlc();
void logout(int haTancatSocketPoole);
int connectBowman(char *tokens[]);

#endif