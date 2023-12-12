#ifndef FRAME_H_
#define FRAME_H_

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <stdio.h>     // For sprintf function
#include <unistd.h>    // For write, getpid and fork functions
#include <string.h>    // For strlen function
#include <stdlib.h>    // For exit function
#include <sys/wait.h>  // For wait function
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>

#define MAX_TOKENS 10
#define FRAME_SIZE 256
#define HEADER_MAX_SIZE 249 // 256 - 1 byte TYPE - 2 bytes HEADER_LENGTH - 4 bytes padding

typedef struct {
    uint8_t type;
    uint16_t header_length;
    char *header;
    char *data; 
} __attribute__((packed)) Frame;

void printaAcknowledge(char buffer[256], Frame *frame); //Nomes es per debugar
int build_frame(Frame *frame, uint8_t type,  char *header,  char *data);
int build_frame2(Frame *frame, uint8_t type, char *header, char *data, size_t data_len);
int receive_frame(int sockfd, Frame *frame);
void print_frame(Frame *frame);
void print_frame2(Frame *frame);
void pad_frame(Frame *frame, char *frame_buffer);
void pad_frame2(Frame *frame, char *frame_buffer, size_t data_len);
void splitFrame(Frame *frame,char *tokens[]);
void fillFrame(char frame_buffer[], uint8_t type,  char *header,  char *data);
void fillFrame2(char frame_buffer[], uint8_t type, char *header, char *data, size_t data_len);
#endif