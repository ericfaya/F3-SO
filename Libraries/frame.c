#include "frame.h"

void printaAcknowledge(char buffer[256], Frame *frame) { //Nomes es per debugar
    
    frame->type = buffer[0];
    frame->header_length = ntohs(*(uint16_t *)&buffer[1]); 

    frame->header = (char *)malloc(frame->header_length + 1); // +1 pel \0
    memcpy(frame->header, &buffer[3], frame->header_length);
    frame->header[frame->header_length] = '\0'; // Null-terminar el header

    size_t data_length = 256 - 3 - frame->header_length;
    frame->data = (char *)malloc(data_length + 1);  // +1 pel \0
    memcpy(frame->data, &buffer[3 + frame->header_length], data_length);
    frame->data[data_length] = '\0'; // Null-terminar el data
}

int build_frame(Frame *frame, uint8_t type,  char *header,  char *data) {
    size_t header_len = strlen(header);
    frame->type = type;
    frame->header_length = htons(header_len); 
    frame->header = (char *)malloc(header_len + 1);    //assignar memoria al header // +1 pel \0
    if (!frame->header) {
        perror("malloc failed for header");
        exit(EXIT_FAILURE);
        return -1;
    }

    strcpy(frame->header, header); 
    size_t sizeOfData = FRAME_SIZE - 3 - header_len - 1;    //calcular tamany dades i assignar memoria// -1 per treure el barra zero.
    frame->data = (char *)malloc(sizeOfData);

    if (!frame->data) {
        perror("malloc failed for data");
        exit(EXIT_FAILURE);
        return -1;
    }
    if (data) {
        strncpy(frame->data, data, sizeOfData - 1);
        frame->data[sizeOfData - 1] = '\0'; // Asegurar \0 d nou
    }
    //print_frame(frame);
    return 0;
}
int build_frame2(Frame *frame, uint8_t type, char *header, char *data, size_t data_len) {
    size_t header_len = strlen(header);
    frame->type = type;
    frame->header_length = htons(header_len); 
    frame->header = (char *)malloc(header_len + 1);
    if (!frame->header) {
        perror("Failed to allocate memory for header");
        return -1;
    }
    strcpy(frame->header, header);

    frame->data = (char *)malloc(data_len);
    if (!frame->data) {
        perror("Failed to allocate memory for data");
        free(frame->header);
        return -1;
    }
    memcpy(frame->data, data, data_len);

    return 0;
}

int receive_frame(int sockfd, Frame *frame) {
    char buffer[256];
    ssize_t bytes_read = 0;
    size_t total_bytes_read = 0;
    while (total_bytes_read < sizeof(buffer)) {
        printf("Debug: llegint del socket...\n");
        bytes_read = recv(sockfd, buffer + total_bytes_read, sizeof(buffer) - total_bytes_read, 0);
        printf("Debug: Bytes recibidos: %ld\n", bytes_read);

        if (bytes_read < 0) {
            write(STDOUT_FILENO, "Error reading from socket\n", sizeof("Error reading from socket\n"));   
            //perror("Error reading from socket"); NOSE SI SE POT UTILITZAR
            return -1;
        } else if (bytes_read == 0) {
            write(STDOUT_FILENO, "Socket closed unexpectedly.\n", sizeof("Socket closed unexpectedly.\n"));   
            //fprintf(stderr, "Socket closed unexpectedly.\n");
            return -1;
        }
        total_bytes_read += bytes_read;
    }
    

    frame->type = buffer[0];
    frame->header_length = ntohs(*(uint16_t *)&buffer[1]); 
    frame->header = (char *)malloc(frame->header_length + 1); // +1 pel \0
    if (frame->header == NULL) {
        fprintf(stderr, "Error allocating memory for frame header.\n");
        return -1; // or another appropriate error code
    }

    memcpy(frame->header, &buffer[3], frame->header_length);
    frame->header[frame->header_length] = '\0'; // Null-terminar el header

    size_t data_length = 256 - 3 - frame->header_length;
    frame->data = (char *)malloc(data_length + 1);  // +1 pel \0
    if (frame->data == NULL) {
        fprintf(stderr, "Error allocating memory for frame header.\n");
        return -1; // or another appropriate error code
    }
    memcpy(frame->data, &buffer[3 + frame->header_length], data_length);
    frame->data[data_length] = '\0'; // Null-terminar el data

    printf("Debug: Trama rebuda. Tipo: %x, Header: %s, Data: %s\n", frame->type, frame->header, frame->data);
   // printf("\n");
    /*printf("Received frame type: 0x%02X\n", frame->type);
    printf("Received frame header length: %u\n", frame->header_length);
    printf("Received frame header: %s\n", frame->header);
    printf("Received frame data: %s\n\n", frame->data);*/
   
    return 0;
}

void print_frame(Frame *frame) {
    printf("Type: 0x%02X\n", frame->type);
    printf("Header Length: %u\n", frame->header_length); 
    printf("Header: %s\n", frame->header);
    printf("Data: %s\n", frame->data);
}

void pad_frame(Frame *frame, char *frame_buffer) {
    frame_buffer[0] = frame->type; // Copiar type
    uint16_t net_header_length = frame->header_length;    // Copiar header_length 
    memcpy(frame_buffer + 1, &net_header_length, sizeof(net_header_length));
    size_t header_len = ntohs(net_header_length);     // copiar header
    memcpy(frame_buffer + 3, frame->header, header_len);
    size_t data_length = FRAME_SIZE - 3 - header_len;    //copiar dades
    memcpy(frame_buffer + 3 + header_len, frame->data, data_length);
    memset(frame_buffer + 3 + header_len + data_length, 0, FRAME_SIZE - (3 + header_len + data_length));    //fotre mes 0s si cal
}

void pad_frame2(Frame *frame, char *frame_buffer, size_t data_len) {
    frame_buffer[0] = frame->type; // Copiar type
    uint16_t net_header_length = frame->header_length; // Copiar header_length 
    memcpy(frame_buffer + 1, &net_header_length, sizeof(net_header_length));
    size_t header_len = ntohs(net_header_length); // copiar header
    memcpy(frame_buffer + 3, frame->header, header_len);

    // Copiar los datos binarios usando data_len
    memcpy(frame_buffer + 3 + header_len, frame->data, data_len);

    // Rellenar el resto del buffer con 0 si es necesario
    size_t total_frame_length = 3 + header_len + data_len;
    if (total_frame_length < FRAME_SIZE) {
        memset(frame_buffer + total_frame_length, 0, FRAME_SIZE - total_frame_length);
    }
}


void splitFrame(Frame *frame,char *tokens[]) {
    char *delimiter = "&";
    char *token = strtok(frame->data, delimiter);
    int num_tokens=0;

    while (token != NULL && num_tokens < MAX_TOKENS) {
        tokens[num_tokens]=token;

        num_tokens++;
        token = strtok(NULL, delimiter);
    }
}

void fillFrame(char frame_buffer[], uint8_t type,  char *header,  char *data) {
    Frame *poole_frame;
    poole_frame = (Frame *)malloc(sizeof(Frame));//cada cop que entradara aqui,amplia una posicio el malloc el realloc
    build_frame(poole_frame, type, header, data);
    pad_frame(poole_frame, frame_buffer);
    free(poole_frame->header);
    free(poole_frame->data);
    free(poole_frame);
}

void fillFrame2(char frame_buffer[], uint8_t type, char *header, char *data, size_t data_len) {
    Frame *poole_frame = (Frame *)malloc(sizeof(Frame));
    if (!poole_frame) {
        perror("Failed to allocate memory for frame");
        return;
    }

    if (build_frame2(poole_frame, type, header, data, data_len) != 0) {
        free(poole_frame);
        return;
    }

    pad_frame2(poole_frame, frame_buffer, data_len);
    free(poole_frame->header);
    free(poole_frame->data);
    free(poole_frame);
}
