#ifndef CONFIG_H_
#define CONFIG_H_
#include <stdio.h>     // For sprintf function
#include <unistd.h>    // For write, getpid and fork functions
#include <string.h>    // For strlen function
#include <stdlib.h>    // For exit function
#include <sys/wait.h>  // For wait function
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>

#include "Poole.h"

char *read_until(int fd, char end);
Poole readUser(int fd);
Poole* readTextFile(char *file, int *numUsuaris);

#endif