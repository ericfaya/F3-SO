
#ifndef DIRFUNCTIONS_H
#define DIRFUNCTIONS_H

#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>

int findSongInDirectory(const char *directory, const char *song_name, char *path_found);
void listSongsInDirectory(char *directory, char *result, int includeDirs);
void listAllSongs(char *directory, char *result);
void listPlayLists(char *directory, char *result);


#endif // SONGFUNCTIONS_H