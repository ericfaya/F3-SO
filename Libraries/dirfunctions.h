
#ifndef DIRFUNCTIONS_H
#define DIRFUNCTIONS_H

#include <dirent.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>


char* findSongInDirectory(const char *directory, const char *song_name) ;
void listSongsInDirectory(char *directory, char *result, int includeDirs);
void listAllSongs(char *directory, char *result);
void listPlayLists(char *directory, char *result);
int ensureUserDirectoryExists(const char *baseDir, const char *userName);


#endif // SONGFUNCTIONS_H