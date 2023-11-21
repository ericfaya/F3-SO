#include "config.h"


char *read_until(int fd, char end){
    int i = 0, size;
    char c = '\0';
    char *string = (char *)malloc(sizeof(char));

    while (1)
    {
        size = read(fd, &c, sizeof(char));

        if (c != end && size > 0)
        {
            string = (char *)realloc(string, sizeof(char) * (i + 2));
            string[i++] = c;
        }
        else
        {
            break;
        }
    }

    string[i] = '\0';
    return string;
}

Bowman readUser(int fd)
{
    Bowman userTmp;
    
    // Name
    /*TODO: El nom dels usuaris no pot contenir ‘&’. En cas que en continguin, s’hauran
d’eliminar. Això és degut al protocol de comunicació del sistema, el qual està a
l’Annex. Aquest serà case sensitive.*/
    userTmp.fullName = read_until(fd, '\n');

    // Path
    userTmp.pathName = read_until(fd, '\n');

    // Ip del server discovery
    userTmp.ipDiscovery = read_until(fd, '\n');

    // port del server discovery
    char *portStrDiscovery = read_until(fd, '\n'); //per no llegir \n i passar 
    userTmp.portDiscovery = atoi(portStrDiscovery);  // Passem de string a int
    free(portStrDiscovery);  // lliberem memoria string

    return userTmp;
}
Bowman* readTextFile(char *file, int *numUsuaris)
{
    int fd, readSize;
    Bowman *bowmaneta;
    char *trash;

    // Try to open the file
    fd = open(file, O_RDONLY);
    if (fd < 0)
    {
        printF("ERROR: File not found\n");
        return NULL;
    }

    // Read user's data
    *numUsuaris = 0;
    bowmaneta = (Bowman *)malloc(sizeof(Bowman));
    while (1)
    {
        bowmaneta = (Bowman *)realloc(bowmaneta, sizeof(Bowman) * (*numUsuaris + 1));//cada cop que entra aqui,amplia una posicio el malloc el realloc
        bowmaneta[*numUsuaris] = readUser(fd);
        *numUsuaris = *numUsuaris + 1;

        // Read EOF
        trash = malloc(sizeof(char));
        readSize = read(fd, trash, 1);
        free(trash);
     
        if (readSize == 0){   // Check if EOF
            break;
        }
    }
    
    close(fd);// Close file
    return bowmaneta;
}