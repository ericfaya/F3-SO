#include "Bowman.h"
#include "config.h"

Bowman* bowmaneta;
int numUsuaris;
char *tokens[MAX_TOKENS];
int sockfd_poole;
<<<<<<< HEAD
int isConnectedToPoole = 0;

pthread_mutex_t socket_mutex = PTHREAD_MUTEX_INITIALIZER;
=======
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
>>>>>>> origin/main


int fillDownloadInfo(const Frame *file_info_frame, FileInfo *downloadInfo) {
   // printf("Iniciando fillDownloadInfo...\n");
    char *token, *dataCopy;

    dataCopy = strdup(file_info_frame->data);

    if (dataCopy == NULL) {
        perror("strdup failed");
        return -1;
    }
    //printf("Data copiada: %s\n", dataCopy);

    // Extracción de FileName
    token = strtok(dataCopy, "&");
    if (token == NULL) {
        free(dataCopy);
        return -1;
    }
    //printf("FileName extraído: %s\n", token);
    downloadInfo->fileName = strdup(token);

    // Extracción de fileSize
    token = strtok(NULL, "&");
    if (token == NULL) { 
        free(downloadInfo->fileName);
        free(dataCopy);
        return -1;
    }
    //printf("FileSize extraído: %s\n", token);
    downloadInfo->fileSize = atoi(token);

    // Extracción de md5sum
    token = strtok(NULL, "&");
    if (token == NULL) {
        free(downloadInfo->fileName);
        free(dataCopy);
        return -1;
    }
    //printf("MD5SUM extraído: %s\n", token);
    downloadInfo->md5sum = strdup(token);

    // Extracción de songId
    token = strtok(NULL, "&");
    if (token == NULL) {
        free(downloadInfo->md5sum);
        free(downloadInfo->fileName);
        free(dataCopy);
        return -1;
    }
    //printf("SongId extraído: %s\n", token);
    downloadInfo->songId = atoi(token);

    free(dataCopy);
    //printf("fillDownloadInfo completado correctamente.\n");
    return 0;
}

void connectDiscovery(char *tokens[]){
    char *buffer;

    char frame_buffer[FRAME_SIZE] = {0};
    fillFrame(frame_buffer,0x01,"NEW_BOWMAN",bowmaneta[0].fullName);
   
    struct sockaddr_in server_addr;    //sockaddr_in: struct defineix l’estructura que permet configurar diversos paràmetres del socket com IP, port
    memset(&server_addr, 0, sizeof(server_addr));//Inicialitza,fica 0s a l'estructura
    server_addr.sin_family = AF_INET;//tipus de familia de socket es tracta
    server_addr.sin_port = htons(bowmaneta[0].portDiscovery);//(Host To Network Short) Converteix port a big endian

    
    if (inet_pton(AF_INET, bowmaneta[0].ipDiscovery, &server_addr.sin_addr) < 0) { //Converteix representació en text de la ip a l’equivalent binari (IPv4)
        perror("Invalid address/ Address not supported");
        //fer free de memoria dinamica
        exit(EXIT_FAILURE);
    }

    int sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); // SOCK_STREAM protocol tcp orientat a connexio//AF_INET familia IPV4
    if (sockfd < 0) {
        perror("Cannot create socket");
        exit(EXIT_FAILURE);
    }

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection Failed");
        //fer free de memoria dinamica
        exit(EXIT_FAILURE);
        close(sockfd); //Per si acas
    }
    
    send(sockfd, frame_buffer, FRAME_SIZE, 0);

    
    
    char info[256];
    read(sockfd, info, 256);

    Frame frameAcknoledge;

    printaAcknowledge(info,&frameAcknoledge);
    splitFrame(&frameAcknoledge,tokens);
    close(sockfd);
    asprintf(&buffer, " %s\n",bowmaneta[0].fullName);
    write (STDOUT_FILENO, buffer, strlen(buffer));

   
    free(buffer);
}

void processSongsResponse(Frame *frame) {
    printf("songs:\n");
    print_frame2(frame);
    // Aquí podrías agregar lógica adicional para manejar los datos de las canciones
}

void processPlaylistsResponse(Frame *frame) {
    printf("playlists:\n");
    print_frame2(frame);
    // Aquí podrías agregar lógica adicional para manejar los datos de las playlists
}

int receiveFileData(int sockfd, int fd_song, ssize_t fileSize) {
    Frame incoming_frame;
    
    ssize_t totalBytesReceived = 0;

    //printf("Inicio de receiveFunctionFileData. fileSize esperado: %zd bytes\n", fileSize);
    while ( totalBytesReceived < fileSize) {
        incoming_frame.header = NULL;
        incoming_frame.data = NULL;
                                     
        int errorSocketOrNot = receive_frame(sockfd, &incoming_frame);
        if (errorSocketOrNot < 0) {
                        printF("Error receiving frame");

            perror("Error receiving frame");
            break;
        }
        //printf("Frame recibido. Header: %s, Data size: %zd\n", incoming_frame.header, strlen(incoming_frame.data));

        if (strcmp(incoming_frame.header, "FILE_DATA") == 0) {
            int idAndSeparatorLength = sizeof(int) + 1;
            char *fileDataStart = incoming_frame.data + idAndSeparatorLength;
            ssize_t data_length = FRAME_SIZE - 3 - incoming_frame.header_length - idAndSeparatorLength;

            if (totalBytesReceived + data_length > fileSize) {
                data_length = fileSize - totalBytesReceived;
            }

            ssize_t bytes_written = write(fd_song, fileDataStart, data_length);
            if (bytes_written == -1) {
                                        printF("Error writing to file");

                perror("Error writing to file");
                free(incoming_frame.header);
                free(incoming_frame.data);
                return -1;
            }

            totalBytesReceived += bytes_written;
            //printf("Received and wrote %zd bytes of data in this frame, total data received: %zd bytes\n", bytes_written, totalBytesReceived);
        } 


        free(incoming_frame.header);
        free(incoming_frame.data);
    }

    //printf("Total data received: %zd bytes\n", totalBytesReceived);
    return (totalBytesReceived == fileSize) ? 0 : -1;
}


void *downloadSongs(void *arg) {
    FileInfo *downloadInfo = (FileInfo *)arg;
    //printf("\nIniciando download Songs. Descargando: %s, fileSize: %d\n", downloadInfo->fileName, downloadInfo->fileSize);
    char songPath[PATH_MAX];
    sprintf(songPath, "%s.mp3", downloadInfo->fileName);

    int fd_song = open(songPath, O_WRONLY | O_CREAT | O_TRUNC, 0666);

    if (fd_song == -1) {
        perror("Error opening file for download");
        free(downloadInfo->fileName);
        free(downloadInfo->md5sum);
        free(downloadInfo);
        pthread_exit(NULL);
        return NULL;
    } 

    if (receiveFileData(sockfd_poole, fd_song, downloadInfo->fileSize) == 0) {
        //printf("Download completed\n");  

        char *calculated_md5 = calculateMD5(songPath);
        if (calculated_md5 != NULL && strcmp(downloadInfo->md5sum, calculated_md5) == 0) {
            write(1,"MD5 verification successful\n",sizeof("MD5 verification successful\n"));

        } else {
            write(1,"MD5 verification failed\n",sizeof("MD5 verification failed\n"));
        }
        free(calculated_md5);
    } else {
        printf("Download failed\n");  
    }

    pthread_mutex_unlock(&socket_mutex); 
    //write(1,"UNLOCK,\n",sizeof("UNLOCK\n"));


    close(fd_song);

    free(downloadInfo->fileName);
    free(downloadInfo->md5sum);
    free(downloadInfo);
    //printf("Finalizando download Songs.\n");
    //pthread_exit(NULL);
    return NULL;
}

void processFileResponse(Frame *frame) {
        
    FileInfo fileInfo;
       
    if (fillDownloadInfo(frame, &fileInfo) != 0) {
        printf("Error processing file info.\n");
        return;
    }

    //printf("File Name: %s ", fileInfo.fileName);
    //printf(" File Size: %d bytes ", fileInfo.fileSize);
    //printf(" MD5 Sum: %s ", fileInfo.md5sum);
    //printf(" Song ID: %d ", fileInfo.songId);

        // nou thread pels Downloads
    FileInfo *threadInfo = malloc(sizeof(FileInfo));
    *threadInfo = fileInfo;
    pthread_t downloadThread;
    if (pthread_create(&downloadThread, NULL, downloadSongs, threadInfo) != 0) {
        perror("Error creating download thread");
    
    }
    //pthread_join(downloadThread,NULL);
  

}


void *socketListener() {
    //printf("Entrant a socketListener.\n");
    while (1) {
        Frame frame;
        pthread_mutex_unlock(&socket_mutex); 
        int readStatus = receive_frame(sockfd_poole, &frame);
        //print_frame(&frame);
        pthread_mutex_lock(&socket_mutex); 

        if (readStatus < 0) {
            printF("Error");
            break;
        }

        if (strcmp(frame.header, "SONGS_RESPONSE") == 0) {
            processSongsResponse(&frame);
        } else if (strcmp(frame.header, "PLAYLISTS_RESPONSE") == 0) {
            processPlaylistsResponse(&frame);
        } else if (strcmp(frame.header, "NEW_FILE") == 0) {
                printf("Download started!\n");

            processFileResponse(&frame); 
            pthread_mutex_lock(&socket_mutex); 
            //write(1,"LOCK,\n",sizeof("LOCK\n"));

        } else if (strcmp(frame.header, "FILE_DATA") == 0) {
            print_frame(&frame);
        }

        free(frame.header);
        free(frame.data);
       

    }
     printf("sortintd de de socketListener.\n");
    return NULL;
}

int connectToPoole(char *tokens[]) {
    

    if (!tokens[1] || !tokens[2]) {
        printf("Error: Información de Poole no disponible.\n");
        return 0;
    }

    char *ipPoole = tokens[1];
    uint16_t portPoole = (uint16_t)strtoul(tokens[2],NULL,10);    //eL STRTOUL converteix la cadena en un valor numeric
    
    char frame_buffer[FRAME_SIZE] = {0};
    fillFrame(frame_buffer,0x01,"NEW_BOWMAN",bowmaneta[0].fullName);


    struct sockaddr_in server_addr;    //sockaddr_in: struct defineix l’estructura que permet configurar diversos paràmetres del socket com IP, port

    memset(&server_addr, 0, sizeof(server_addr));//Inicialitza,fica 0s a l'estructura
    server_addr.sin_family = AF_INET;//tipus de familia de socket es tracta
    server_addr.sin_port = htons(portPoole);//(Host To Network Short) Converteix port a big endiaN
   
    if (inet_pton(AF_INET, ipPoole, &server_addr.sin_addr) < 0) { //Converteix representació en text de la ip a l’equivalent binari (IPv4)
        perror("Invalid address/ Address not supported");
        //fer free de memoria dinamica
        exit(EXIT_FAILURE);
    }
    
    sockfd_poole = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);// SOCK_STREAM protocol tcp orientat a connexio     //AF_INET familia IPV4
    if (sockfd_poole < 0) {
        perror("Cannot create socket");
        exit(EXIT_FAILURE);
    }

    if (connect(sockfd_poole, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection Failed");
        //fer free de memoria dinamica
        exit(EXIT_FAILURE);
        close(sockfd_poole); //Per si acas
    }
    send(sockfd_poole, frame_buffer, FRAME_SIZE, 0); 
    Frame frameAcknowledge;
    if (receive_frame(sockfd_poole, &frameAcknowledge) < 0) {
        perror("Error al leer respuesta de Poole");
        close(sockfd_poole);
        return 0;
    }
 

    if (strcmp(frameAcknowledge.header, "CON_OK") == 0) {
        // LA Connexio amb Poole es bona, per tant, llancem thread per escoltar trames.
        pthread_t listenerThread;
        if (pthread_create(&listenerThread, NULL, socketListener, NULL) != 0) {
            perror("Error al crear el hilo de escucha");
            
            close(sockfd_poole);
            return 0;
        }
        return 1;
    }
    else{
        
        close(sockfd_poole);
        return 0;
    }

}

int connectBowman(char *tokens[MAX_TOKENS]){ 
    connectDiscovery(tokens);
    printF(" connected to HAL 9000 system, welcome music lover!\n");
    return connectToPoole(tokens);
      
}

<<<<<<< HEAD
void listSongs(int *connectedOrNot) {
    if(*connectedOrNot){
        char frame_buffer[FRAME_SIZE] = {0};
        fillFrame(frame_buffer,0x02,"LIST_SONGS"," ");
        
        send(sockfd_poole, frame_buffer, FRAME_SIZE, 0);//Bowman send poole
         pthread_mutex_unlock(&socket_mutex);
=======
void *controleCommandsWrapper(void *args) {
    ThreadArgs *threadArgs = (ThreadArgs *)args;
    int result = controleCommands(threadArgs->whichCommand, &(threadArgs->connectedOrNot));
    threadArgs->connectedOrNot=result;
    return (void *)(intptr_t)result;
}

int receiveFileData(int sockfd, int fd_song, ssize_t fileSize) {
    Frame incoming_frame;
    int fileCompleted = 0;
    ssize_t totalBytesReceived = 0;

    //printf("Inicio de receiveFileData. fileSize esperado: %zd bytes\n", fileSize);

    while (!fileCompleted && totalBytesReceived < fileSize) {
        incoming_frame.header = NULL;
        incoming_frame.data = NULL;
pthread_mutex_lock(&mutex);
                                              
        int errorSocketOrNot = receive_frame(sockfd, &incoming_frame);
        if (errorSocketOrNot < 0) {
            perror("Error receiving frame");
            break;
        }
  pthread_mutex_unlock(&mutex);
        //printf("Frame recibido. Header: %s, Data size: %zd\n", incoming_frame.header, strlen(incoming_frame.data));

        if (strcmp(incoming_frame.header, "FILE_DATA") == 0) {
            int idAndSeparatorLength = sizeof(int) + 1;
            char *fileDataStart = incoming_frame.data + idAndSeparatorLength;
            ssize_t data_length = FRAME_SIZE - 3 - incoming_frame.header_length - idAndSeparatorLength;

            if (totalBytesReceived + data_length > fileSize) {
                data_length = fileSize - totalBytesReceived;
            }

            ssize_t bytes_written = write(fd_song, fileDataStart, data_length);
            if (bytes_written == -1) {
                perror("Error writing to file");
                free(incoming_frame.header);
                free(incoming_frame.data);
                return -1;
            }

            totalBytesReceived += bytes_written;
          //  printf("Received and wrote %zd bytes of data in this frame, total data received: %zd bytes\n", bytes_written, totalBytesReceived);
        } 

        free(incoming_frame.header);
        free(incoming_frame.data);
>>>>>>> origin/main
    }
    else{
        printF("Cannot list, you are not connected to HAL 9000\n");
    } 
}



void listPlaylists(int *connectedOrNot) {
     if(*connectedOrNot){
        char frame_buffer[FRAME_SIZE] = {0};
        fillFrame(frame_buffer,0x06,"LIST_PLAYLISTS"," ");

        send(sockfd_poole, frame_buffer, FRAME_SIZE, 0);//Bowman send poole
        pthread_mutex_unlock(&socket_mutex);

    }
    else{
        printF("Cannot list, you are not connected to HAL 9000\n");
    }
}


void download(int *connectedOrNot, char *commandInput) {
    if (*connectedOrNot == 1) {
        
        char *song_name = commandInput + strlen("DOWNLOAD ");
        if (strlen(song_name) == 0) {
            printF("No song name provided\n");
            return;
        }
        char frame_buffer[FRAME_SIZE] = {0};
        fillFrame(frame_buffer, 0x03, "DOWNLOAD_SONG", song_name);
        send(sockfd_poole, frame_buffer, FRAME_SIZE, 0);

<<<<<<< HEAD
=======
        

       
>>>>>>> origin/main
    } else {
        printF("Not connected to HAL 9000\n");
    }
}



<<<<<<< HEAD
int controleCommands(char *whichCommand, int *connectedOrNot) {
=======
        if (fillDownloadInfo(file_info_frame, &downloadInfo) != 0) {
            printf("Error en fillDownloadInfo.\n");
            free(file_info_frame->header);
            free(file_info_frame->data);
            return;
        }
        //printf("Preparando para crear el hilo downloadSongs.\n");
        FileInfo *threadInfo = malloc(sizeof(FileInfo));
        *threadInfo = downloadInfo;

        pthread_t t1;
        int s = pthread_create(&t1, NULL, downloadSongs, threadInfo);
        if (s != 0) {
            printf("pthread_create failed\n");
            free(threadInfo->fileName);
            free(threadInfo->md5sum);
            free(threadInfo);
            free(file_info_frame->header);
            free(file_info_frame->data);
           // exit(EXIT_FAILURE);Erfaes888
        }
                
       // void *res;
       // pthread_join (t1,&res);
    }
}

int controleCommands(char whichCommand[50],int *connectedOrNot) {
>>>>>>> origin/main
    int flag=0;
    char *whichCommand1,*whichCommand2;
    const char delimiter = ' ';
    whichCommand1=strtok(whichCommand, &delimiter);

   
        if(whichCommand1 != NULL){
            if((strcasecmp("CONNECT",whichCommand1) == 0) && *connectedOrNot == 0){//Segona condicio per que no es connecti mes d'un cop
                whichCommand2=strtok(NULL, &delimiter);//Si li fiquem NULL començara la segona busqueda per on es va quedar cuan es va cridar per primer cop strtok
                if(whichCommand2 == NULL){
                    *connectedOrNot=connectBowman(tokens);
                }
                else{
                    printF("Unknown command\n");
                } 
                flag=1;
            }

            if(strcasecmp("LOGOUT",whichCommand1) == 0){//TODO F2
                //logout();
                return 0;
            }

            if(strcasecmp("LIST",whichCommand1) == 0){//TODO F2
                whichCommand2=strtok(NULL, &delimiter);//Si li fiquem NULL començara la segona busqueda per on es va quedar cuan es va cridar per primer cop strtok
                if(whichCommand2 != NULL){
                    if(strcasecmp("SONGS",whichCommand2) == 0){
                        listSongs(connectedOrNot); 
<<<<<<< HEAD
=======
                        pthread_mutex_lock(&mutex);

                        errorSocketOrNot = receive_frame(sockfd_poole, &incoming_frame);
                        if (errorSocketOrNot >= 0) {
                            print_frame(&incoming_frame);

                        }
                        pthread_mutex_unlock(&mutex);

>>>>>>> origin/main
                        flag=1;
                    }
                    else if(strcasecmp("PLAYLISTS",whichCommand2) == 0){
                        listPlaylists(connectedOrNot);
<<<<<<< HEAD
=======
                        pthread_mutex_lock(&mutex);

                        errorSocketOrNot = receive_frame(sockfd_poole, &incoming_frame);
                        if (errorSocketOrNot >= 0) {
                            print_frame(&incoming_frame);

                        }
                        pthread_mutex_unlock(&mutex);

>>>>>>> origin/main
                        flag=1;
                    }
                }
            }

            if(strcasecmp(whichCommand1,"DOWNLOAD") == 0){ //TODO F3
                //write(1,"Jambele\n",sizeof("Jambele\n"));
<<<<<<< HEAD
                download(connectedOrNot, whichCommand);
=======
                //print_frame(&incoming_frame);
                download(connectedOrNot, whichCommand);
                pthread_mutex_lock(&mutex);

            //print_frame(&incoming_frame);
                errorSocketOrNot = receive_frame(sockfd_poole, &incoming_frame);
                pthread_mutex_unlock(&mutex);
                print_frame(&incoming_frame);
                if (errorSocketOrNot >= 0) {
                    if(strcasecmp(whichCommand1,"DOWNLOAD") == 0){
                        pthread_mutex_lock(&mutex);
                        download2(&incoming_frame);
                        pthread_mutex_unlock(&mutex);

                    }
                }
                

>>>>>>> origin/main
                flag=1; 
            }

            if(strcasecmp("CHECK",whichCommand1) == 0){//TODO F3
                //checkDownload(connectedOrNot);
                flag=1;
            }

            if(strcasecmp("CLEAR",whichCommand1) == 0){//TODO F3
                //clearDownload(connectedOrNot);
                flag=1;
            }
            /*pthread_mutex_lock(&mutex);

            //print_frame(&incoming_frame);
            errorSocketOrNot = receive_frame(sockfd_poole, &incoming_frame);
            pthread_mutex_unlock(&mutex);
            print_frame(&incoming_frame);
            if (errorSocketOrNot >= 0) {
                if(strcasecmp(whichCommand1,"DOWNLOAD") == 0){
                    download2(&incoming_frame);
                }
            }*/
            


            else if(flag==0){
                printF("ERROR: Please input a valid command.\n");
            }
        }

        return 1;
}


void freeMemory(Bowman* bowmaneta, int numUsuaris){
    int i;
    for(i=0; i<numUsuaris; i++){
        free(bowmaneta[i].fullName);
        free(bowmaneta[i].pathName);
        free(bowmaneta[i].ipDiscovery);
        
    }
    free(bowmaneta);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("ERROR: Incorrect number of arguments\n");
        return -1;
    }

    bowmaneta = readTextFile(argv[1], &numUsuaris);
    if (bowmaneta == NULL) {   
        return -2;
    }


    char *command;
    int connectedOrNot = 0;
    write(1, "\n$", 3);
    while (1) {
        /** 1R THREAD ESCOLTAR DE LA TERMINAL*/ //canvi fet que deien els becaris
       
        command = read_until(STDIN_FILENO, '\n'); // Leer el comando del usuario
        if (command == NULL || strlen(command) == 0) {
            free(command); 
            continue; 
        }
        controleCommands(command, &connectedOrNot);
        write(1, "\n$", 3);
        free(command); 
    }

   //lliberar memoria pero no entenc el numUsuaris??? TODO CANVIARHO

    return 0;
}

