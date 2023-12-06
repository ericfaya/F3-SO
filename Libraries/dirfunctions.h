
#ifndef DIRFUNCTIONS_H
#define DIRFUNCTIONS_H

int findSongInDirectory(const char *directory, const char *song_name, char *path_found);
void listSongsInDirectory(char *directory, char *result, int includeDirs);
void listAllSongs(char *directory, char *result);
void listPlayLists(char *directory, char *result);

#endif // SONGFUNCTIONS_H