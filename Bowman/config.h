#ifndef CONFIG_H
#define CONFIG_H
#include <stdio.h>     // For sprintf function
#include <unistd.h>    // For write, getpid and fork functions
#include <string.h>    // For strlen function
#include <stdlib.h>    // For exit function
#include <sys/wait.h>  // For wait function
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>

#include "Bowman.h"

char *read_until(int fd, char end);
Bowman readUser(int fd);
Bowman* readTextFile(char *file, int *numUsuaris);

#endif