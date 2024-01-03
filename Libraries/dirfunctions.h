
#ifndef DIRFUNCTIONS_H
#define DIRFUNCTIONS_H

#include <dirent.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>


char* findSongInDirectory(const char *directory, const char *song_name) ;
void listSongsInDirectory(char *directory, char *result, int includeDirs);
void listAllSongs(char *directory, char *result);
void listPlayLists(char *directory, char *result);


#endif // SONGFUNCTIONS_H