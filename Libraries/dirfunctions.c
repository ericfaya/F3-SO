#include "dirfunctions.h"
#include <dirent.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>

int findSongInDirectory(const char *directory, const char *song_name, char *path_found) {
    DIR *dir;
    struct dirent *entry;
    char path[1024];
    int found = 0;

    dir = opendir(directory);
    if (dir == NULL) {
        perror("No se pudo abrir el directorio");
        return 0;
    }

    while (!found && (entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') {
            continue;
        }
        snprintf(path, sizeof(path), "%s/%s", directory, entry->d_name);

        struct stat path_stat;
        stat(path, &path_stat);
        if (S_ISDIR(path_stat.st_mode)) {
            found = findSongInDirectory(path, song_name, path_found);
        } else {
            
            char *dot = strstr(entry->d_name, ".mp3");
            if (dot && strcmp(dot, ".mp3") == 0) {
                char nomSenseExt[dot - entry->d_name + 1];
                strncpy(nomSenseExt, entry->d_name, dot - entry->d_name);
                nomSenseExt[dot - entry->d_name] = '\0';

                
                if (strcmp(nomSenseExt, song_name) == 0 || strcmp(entry->d_name, song_name) == 0) {
                    strcpy(path_found, path); 
                    found = 1;
                }
            }
        }
    }
    closedir(dir);
    return found;
}



void listSongsInDirectory(char *directory, char *result, int includeDirs) {
    DIR *directori;
    struct dirent *entrada;
    char path[1024];

    directori = opendir(directory);
    if (directori != NULL) {
        while ((entrada = readdir(directori)) != NULL) {
            if (entrada->d_name[0] == '.') {
                continue;
            }
            snprintf(path, sizeof(path), "%s/%s", directory, entrada->d_name);
            
            struct stat path_stat;
            stat(path, &path_stat);
            if (S_ISDIR(path_stat.st_mode)) {
                if (includeDirs) {
                    strcat(result, entrada->d_name);
                    strcat(result, "&");
                }
                listSongsInDirectory(path, result, includeDirs);
                if (includeDirs) {
                    strcat(result, "#");
                }
            } else if (strstr(entrada->d_name, ".mp3")) {
                size_t mida = strlen(entrada->d_name) - 4;
                //char nomSenseExt[mida];
                char nomSenseExt[mida + 1];
                strncpy(nomSenseExt, entrada->d_name, mida);
                nomSenseExt[mida] = '\0';
                strcat(result, nomSenseExt);
                strcat(result, "&");
            }
        }
        closedir(directori);
    } else {
        perror("No es pot obrir el directori");
    }
}

void listAllSongs(char *directory, char *result) {
    strcpy(result, "");
    listSongsInDirectory(directory, result, 0); // 0 per no ficar nom d llistes (directoris)
    size_t mida = strlen(result);
    if (mida > 0 && result[mida - 1] == '&') { //evitem un & o # al final
        result[mida - 1] = '\0';
    }
}

void listPlayLists(char *directory, char *result) {
    strcpy(result, "");
    listSongsInDirectory(directory, result, 1); // 1 per ficar noms d llistes
    size_t mida = strlen(result);

    while (mida > 0 && (result[mida - 1] == '#' || result[mida - 1] == '&')) { //evitem un & o # al final
        result[mida - 1] = '\0';
        mida--;
    }
}

