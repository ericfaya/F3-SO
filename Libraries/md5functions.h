// md5functions.h
#ifndef MD5FUNCTIONS_H
#define MD5FUNCTIONS_H
#include <fcntl.h> // para open
#include <unistd.h> // para read, close, pipe, fork
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h> 

char *calculateMD5(const char *filename);
int verifyMD5SUM(const char *file_path, const char *expected_md5);

#endif // MD5FUNCTIONS_H
