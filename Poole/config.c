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

Poole readUser(int fd)
{
    Poole userTmp;
    
    // Name
    userTmp.fullName = read_until(fd, '\n');

    // Path
    userTmp.pathName = read_until(fd, '\n');

    // Ip del server discovery
    userTmp.ipDiscovery = read_until(fd, '\n');
    userTmp.fullName[strlen(userTmp.fullName) -1]='\0';
    // Port del server discovery
    char *portStrDiscovery = read_until(fd, '\n'); //per no llegir \n i passar 
    userTmp.portDiscovery = atoi(portStrDiscovery);  // Passem de string a int
    free(portStrDiscovery);  // lliberem memoria string

    // Ip del server Poole
    userTmp.ipPoole = read_until(fd, '\n');
    userTmp.ipPoole[strlen(userTmp.ipPoole) -1]='\0';



    // Port del server Poole
    char *portStrPoole = read_until(fd, '\n');  //per no llegir \n i passar 
    userTmp.portPoole = atoi(portStrPoole);   // Passem de string a int
    free(portStrPoole);  // lliberem memoria string

    return userTmp;
}

Poole* readTextFile(char *file, int *numUsuaris)
{
    int fd, readSize;
    Poole *poolete;
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
    poolete = (Poole *)malloc(sizeof(Poole));
    while (1)
    {
        poolete = (Poole *)realloc(poolete, sizeof(Poole) * (*numUsuaris + 1));//cada cop que entra aqui,amplia una posicio el malloc el realloc
        poolete[*numUsuaris] = readUser(fd);
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
    return poolete;
}