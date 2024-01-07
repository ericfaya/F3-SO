#include "dirfunctions.h"

char* my_strdup(const char *s) {
    size_t len = strlen(s) + 1;
    char *dup = malloc(len);
    if (dup != NULL) {
        strcpy(dup, s);
    }
    return dup;
}
void findSongsInList(const char *directory, PathList *resultList) {
    DIR *dir;
    struct dirent *entry;

    dir = opendir(directory);
    if (dir == NULL) {
        perror("No se pudo abrir el directorio");
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') {
            continue;
        }

        size_t path_length = strlen(directory) + strlen(entry->d_name) + 2;  // +1 for '/' and +1 for null terminator
        char *path = (char *)malloc(path_length);
        if (path == NULL) {
            perror("Error allocating memory for path");
            closedir(dir);
            return;
        }

        snprintf(path, path_length, "%s/%s", directory, entry->d_name);
        struct stat path_stat;
        stat(path, &path_stat);
        
        if (S_ISDIR(path_stat.st_mode)) {
            // Omitir directorios, solo estamos interesados en archivos
            continue;
        } else {
            char *dot = strstr(entry->d_name, ".mp3");
            if (dot && strcmp(dot, ".mp3") == 0) {

                addToPathList(resultList, path,entry->d_name);
            }
        }
        free(path);
    }
    closedir(dir);
}
//Funcio recursiva per recorre cansons en
char* findSongInDirectory(const char *directory, const char *song_name) {
    DIR *dir;
    struct dirent *entry;
    char *path = NULL;
    char *found_path = NULL;

    //printf("Fucking directory: %s",directory);
    dir = opendir(directory);
    if (dir == NULL) {
        perror("No se pudo abrir el directorio");
        return NULL;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') {
            continue;
        }
        size_t path_length = strlen(directory) + strlen(entry->d_name) + 2;  // +1 for '/' and +1 for null terminator   
        path = (char *)malloc(path_length);
        if (path == NULL) {
            perror("Error allocating memory for path");
            closedir(dir);
            //return 0;
        }

        snprintf(path, path_length, "%s/%s", directory, entry->d_name);
        struct stat path_stat;
        stat(path, &path_stat);
        if (S_ISDIR(path_stat.st_mode)) {
           // free(path);
            found_path = findSongInDirectory(path, song_name);
            if (found_path != NULL) {
                free(path);
                break;
            }        
        } else {
            
            char *dot = strstr(entry->d_name, ".mp3");
            if (dot && strcmp(dot, ".mp3") == 0) {
                char nomSenseExt[dot - entry->d_name + 1];
                strncpy(nomSenseExt, entry->d_name, dot - entry->d_name);
                nomSenseExt[dot - entry->d_name] = '\0';

                
                if (strcmp(nomSenseExt, song_name) == 0 || strcmp(entry->d_name, song_name) == 0) {
                    found_path = my_strdup(path);//found_path = strdup(path);  //strcpy(path_found, path); 
                    //printf("Found path: %s\n", found_path);
                    free(path);
                    //found = 1;
                    break;
                }
            }
        }
        free(path);
    }
    

    closedir(dir);
    return found_path;
}



void listSongsInDirectory(char *directory, char *result, int includeDirs) {
    DIR *directori;
    struct dirent *entrada;
    char *path = NULL;

    directori = opendir(directory);
    if (directori != NULL) {
        while ((entrada = readdir(directori)) != NULL) {
            if (entrada->d_name[0] == '.') {
                continue;
            }
            size_t path_length = strlen(directory) + strlen(entrada->d_name) + 2;  // +1 for '/' and +1 for null terminator

            path = (char *)malloc(path_length);
            if (path == NULL) {
                perror("Error allocating memory for path");
                closedir(directori);
                //return 0;
            }
        

            snprintf(path, path_length, "%s/%s", directory, entrada->d_name);

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


int ensureUserDirectoryExists(const char *baseDir, const char *userName) {
    size_t fullPathLength = strlen(baseDir) + strlen(userName) + 2; // +1 for '/' and +1 for '\0'
    char *fullPath = (char *)malloc(fullPathLength);

    if (fullPath == NULL) {
        perror("Error al asignar memoria para fullPath");
        return -3; 
    }

    //ruta del directori
    snprintf(fullPath, fullPathLength, "%s/%s", baseDir, userName);

    struct stat statbuf;
    if (stat(fullPath, &statbuf) == -1) { //comprovem si existeix
        // si no existeix creem un amb mkdir
        if (mkdir(fullPath, 0777) == -1) { 
            perror("Error al crear directorio de usuario");
            free(fullPath);
            return -1; 
        }
        free(fullPath);
        return 1; //directori creat
    }

    if (!S_ISDIR(statbuf.st_mode)) {
        fprintf(stderr, "Existe un archivo que no es un directorio con el mismo nombre: %s\n", fullPath);
        free(fullPath);
        return -2; // Retorna -2 si hi ha el mateix nom a un arxiu que no sigui directori, no hauria de passar mai
    }

    free(fullPath);
    return 0; // directori ja existeix, es guardara sense crear un d nou
}

void initializePathList(PathList *pathList, size_t initialCapacity) {
    pathList->paths = (char **)malloc(initialCapacity * sizeof(char *));
    pathList->songs = (char **)malloc(initialCapacity * sizeof(char *));

    pathList->size = 0;
    pathList->capacity = initialCapacity;
}

// Agregar un path a la lista
void addToPathList(PathList *pathList, const char *path,const char *song) {
    if (pathList->size == pathList->capacity) {
        pathList->capacity *= 2;
        pathList->paths = (char **)realloc(pathList->paths, pathList->capacity * sizeof(char *));
        pathList->songs = (char **)realloc(pathList->songs, pathList->capacity * sizeof(char *));

    }
    pathList->paths[pathList->size] = my_strdup(path);
    pathList->songs[pathList->size] = my_strdup(song);
    pathList->size++;
}

// Liberar la memoria utilizada por la lista de paths
void freePathList(PathList *pathList) {
    for (size_t i = 0; i < pathList->size; ++i) {
        free(pathList->paths[i]);
        free(pathList->songs[i]);

    }
    free(pathList->paths);
    free(pathList->songs);

    pathList->size = 0;
    pathList->capacity = 0;
}