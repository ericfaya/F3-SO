#ifndef CONFIG_H
#define CONFIG_H

#include "Discovery.h"

char *read_until(int fd, char end);
Discovery readUser(int fd);
Discovery* readTextFile(char *file, int *numUsuaris);

#endif