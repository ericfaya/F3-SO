#include <stdio.h>     // For sprintf function
#include <unistd.h>    // For write, getpid and fork functions
#include <string.h>    // For strlen function
#include <stdlib.h>    // For exit function
#include <sys/wait.h>  // For wait function
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>

#include "config.h"


char *read_until(int fd, char end){
    int i = 0, size;
    char c = '\0';
    char *string = (char *)malloc(sizeof(char));

    if (!string) {
        perror("Unable to allocate memory");
        exit(EXIT_FAILURE);
    }

    while (1)
    {
        size = read(fd, &c, sizeof(char));

        if (c != end && size > 0)
        {
            string = (char *)realloc(string, sizeof(char) * (i + 2));
            if (!string) {
                perror("Unable to allocate memory");
                exit(EXIT_FAILURE);
            }
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

Discovery readUser(int fd)
{
    Discovery userTmp;
  
    // Ip del server Poole
    userTmp.ipPoole = read_until(fd, '\n');

    // Port del server Poole
    char *portStrPoole = read_until(fd, '\n'); //per no llegir \n i passar 
    userTmp.portPoole = atoi(portStrPoole);  // Passem de string a int
    free(portStrPoole);  // lliberem memoria string

    // Ip del usuari bowman
    userTmp.ipBowman = read_until(fd, '\n');

    // Port del server Bowman
    char *portStrBowman = read_until(fd, '\n'); //per no llegir \n i passar 
    userTmp.portBowman = atoi(portStrBowman);  // Passem de string a int
    free(portStrBowman);  // lliberem memoria string

    return userTmp;
}
Discovery* readTextFile(char *file, int *numUsuaris)
{
    int fd, readSize;
    Discovery *discovery;
    char trash;

    // Try to open the file
    fd = open(file, O_RDONLY);
    if (fd < 0)
    {
        printF("ERROR: File not found\n");
        return NULL;
    }

    // Read user's data
    *numUsuaris = 0;
    discovery = (Discovery *)malloc(sizeof(Discovery));
    while (1)
    {
        discovery = (Discovery *)realloc(discovery, sizeof(Discovery) * (*numUsuaris + 1));//cada cop que entra aqui,amplia una posicio el malloc el realloc
        discovery[*numUsuaris] = readUser(fd);
        *numUsuaris = *numUsuaris + 1;

        // Read EOF
        
        readSize = read(fd, &trash, 1);
    
        if (readSize == 0){   // Check if EOF
            break;
        }
        
    }
    
    close(fd);// Close file
    return discovery;
}