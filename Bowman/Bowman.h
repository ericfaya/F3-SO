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
#include "dirfunctions.h"

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
    int sockfd_poole;
} FileInfo;

typedef struct {
    long mtype;
    Frame frame;
}MessageQueue;

typedef struct {
    int mq_id;
    int newCommand;
} ThreadArgs;

typedef struct SongNode {
    FileInfo *fileInfo;//Ho fem constant ja que no voldrem modificar res,nomes veure informacio
    struct SongNode* next;
} SongNode;

void printBarraProgres(FileInfo *fileInfo);
void printAllSongs();
void addSong(FileInfo *fileInfo);
void removeAllSongs();
int fillDownloadInfo(const Frame *file_info_frame, FileInfo *downloadInfo);
void connectDiscovery(char *tokens[]);
void processSongsResponse(Frame *frame);
void processPlaylistsResponse(Frame *frame);
int writeBinaryFile(Frame incoming_frame, FileInfo *downloadInfo);
int receiveFileData(FileInfo *downloadInfo);
void *downloadSongs(void *arg);
void createBinaryFile(Frame *frame, FileInfo *fileInfo);
void processFileResponse(FileInfo *fileInfo);
void messageQueue(Frame *frame, int mq_id, int id_bustia);
void *socketListener(void *arg);
int connectToPoole(char *tokens[]);
int connectBowman(char *tokens[]);
void listSongs();
void listPlaylists();
void download(char *commandInput, char *songsOrPlaylist);
void logoutDiscovery();
void freeMemory();
void logout(int haTancatSocketPoole);
void kctrlc();
int controleCommands(char *whichCommand, int *connectedOrNot);

#endif