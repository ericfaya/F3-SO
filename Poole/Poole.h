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
#include <limits.h>
#include <sys/param.h>

#include "frame.h"
#include "md5functions.h"
#include "dirfunctions.h"
#include "semaphore_v2.h"

#define printF(x) write(1, x, strlen(x))

typedef struct
{
    char *fullName;
    char *pathName;
    char *ipDiscovery;
    int portDiscovery;
    char *ipPoole;
    int portPoole;
} Poole;

typedef struct ThreadArgs {
    int socket;
    int fd_write;  // Añadir este campo para el extremo de escritura del pipe
} ThreadArgs;

typedef struct {
    int socket;
    char *filePath;   // Flexible array member
    char *song_name;  // Flexible array member
    int id;
    char *header;
} FileTransferInfo;

typedef struct ClientNode {
    int sockfd;
    struct ClientNode* next;
} ClientNode;

void removeAllClients();
void kctrlc();
void addClient(int sockfd);
void sendSongListResponse(int socket);
void sendPlayListResponse(int socket);
void *sendFileData(void *arg);
void enviarAcknowledge(int newsock, ssize_t bytes_read);
FileTransferInfo *initializeFileTransferInfo(const char *filePath, const char *songName, int socket, int id, const char *header);
int downloadSong(int socket, char *path_found, char *song_name, const char *header);
int handleBowmanConnection(int *newsock,ssize_t bytes_read/*, int errorSocketOrNot*/, Frame *incoming_frame, int fd_write, PathList *resultList);
void *clientHandler(void *args);
void connectToBowman(Poole *poolete, int fd_write);
void procesoMonolit(int read_fd);


#endif