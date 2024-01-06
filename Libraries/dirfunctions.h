
#ifndef DIRFUNCTIONS_H
#define DIRFUNCTIONS_H

#include <dirent.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>

typedef struct {
    char **paths;
    char **songs;
    size_t size;
    size_t capacity;
} PathList;

char* findSongInDirectory(const char *directory, const char *song_name) ;
void listSongsInDirectory(char *directory, char *result, int includeDirs);
void listAllSongs(char *directory, char *result);
void listPlayLists(char *directory, char *result);
int ensureUserDirectoryExists(const char *baseDir, const char *userName);
void findSongsInList(const char *directory, PathList *resultList) ;
void initializePathList(PathList *pathList, size_t initialCapacity);
void addToPathList(PathList *pathList, const char *path,const char *song);
void freePathList(PathList *pathList);

#endif // SONGFUNCTIONS_H