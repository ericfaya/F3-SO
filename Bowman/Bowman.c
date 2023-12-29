#include "Bowman.h"
#include "config.h"
Bowman* bowmaneta;
int numUsuaris;
char *tokens[MAX_TOKENS];
int sockfd_poole;
int isConnectedToPoole = 0;
int mq_id_queue;
pthread_mutex_t socket_mutex = PTHREAD_MUTEX_INITIALIZER;

int tocaTancar=1;
semaphore sem;
pthread_t listenerThread;
pthread_t downloadThread;

int fillDownloadInfo(const Frame *file_info_frame, FileInfo *downloadInfo) {
    char *token, *dataCopy;

    dataCopy = strdup(file_info_frame->data);
    if (dataCopy == NULL) {
        perror("strdup failed");
        return -1;
    }

    token = strtok(dataCopy, "&");    // Extracción de FileName
    if (token == NULL) {
        free(dataCopy);
        return -1;
    }
    downloadInfo->fileName = strdup(token);

    token = strtok(NULL, "&");    // Extracción de fileSize
    if (token == NULL) { 
        free(downloadInfo->fileName);
        free(dataCopy);
        return -1;
    }
    downloadInfo->fileSize = atoi(token);

    token = strtok(NULL, "&");    // Extracción de md5sum
    if (token == NULL) {
        free(downloadInfo->fileName);
        free(dataCopy);
        return -1;
    }
    downloadInfo->md5sum = strdup(token);

    token = strtok(NULL, "&");    // Extracción de songId
    if (token == NULL) {
        free(downloadInfo->md5sum);
        free(downloadInfo->fileName);
        free(dataCopy);
        return -1;
    }
    downloadInfo->songId = atoi(token);

    free(dataCopy);
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
        exit(EXIT_FAILURE);//fer free de memoria dinamica
    }

    int sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); // SOCK_STREAM protocol tcp orientat a connexio//AF_INET familia IPV4
    if (sockfd < 0) {
        perror("Cannot create socket");
        exit(EXIT_FAILURE);
    }

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection Failed");
        exit(EXIT_FAILURE);  //fer free de memoria dinamica
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
    printF("songs:\n");
    print_frame2(frame);
}
void processPlaylistsResponse(Frame *frame) {
    printF("playlists:\n");
    print_frame2(frame);
}

int writeBinaryFile(Frame incoming_frame,FileInfo *downloadInfo) {
    int idAndSeparatorLength = sizeof(int) + 1;
    char *fileDataStart = incoming_frame.data + idAndSeparatorLength;
    ssize_t data_length = FRAME_SIZE - 3 - incoming_frame.header_length - idAndSeparatorLength;
    if (downloadInfo->totalBytesReceived + data_length > downloadInfo->fileSize) {
        data_length = downloadInfo->fileSize - downloadInfo->totalBytesReceived;
    }
    ssize_t bytes_written = write(downloadInfo->fd_song, fileDataStart, data_length);
    if (bytes_written == -1) {
        printF("Error writing to file");
        perror("Error writing to file");
        free(incoming_frame.header);
        free(incoming_frame.data);
        return -1;
    }
    
    //printf("Received and wrote %zd bytes of data in this frame, total data received: %zd bytes\n", bytes_written, downloadInfo->totalBytesReceived);
    downloadInfo->totalBytesReceived += bytes_written;
    return 0;
}
int receiveFileData( FileInfo *downloadInfo) {
    downloadInfo->totalBytesReceived = 0;
    MessageQueue msg;

    while (downloadInfo->totalBytesReceived < downloadInfo->fileSize && tocaTancar == 1) {        //printf("\n\nRebo per la cua %d + id bustia : %d ",downloadInfo->id_queue,downloadInfo->id_bustia);
        if (msgrcv(downloadInfo->id_queue, &msg, sizeof(MessageQueue)- sizeof(long) , downloadInfo->id_bustia, 0) != -1) {// if (msgrcv(downloadInfo->id_queue, &msg, sizeof(msg) - sizeof(long), downloadInfo->id_bustia, 0) != -1) {
        //  printf("Rebo per la cua %d + id bustia : %ld ",downloadInfo->id_queue,msg.mtype);
        // printf("Receiving data: %s\n\n", msg.frame.data);

        if(writeBinaryFile(msg.frame,downloadInfo)==-1)
            return -1; //Error al escriure al file binary
        //printf("Received and wrote %zd bytes of data in this frame, total data received: %zd bytes\n", bytes_written, downloadInfo->totalBytesReceived);
        }
        else{
            printF("Error al rewbre el mensaje\n");
        }
        //printf("Total data received: %zd bytes\n", downloadInfo->totalBytesReceived);
    }
    
    return (downloadInfo->totalBytesReceived == downloadInfo->fileSize) ? 0 : -1;
}
void *downloadSongs(void *arg) {
    FileInfo *downloadInfo = (FileInfo *)arg;
   
    printf("ID Queue: %d\n", downloadInfo->id_queue);
    printf("ID Bustia: %d\n", downloadInfo->id_bustia);


     if (receiveFileData( downloadInfo) == 0){ // You can also print frame information if needed

        char *calculated_md5 = calculateMD5(downloadInfo->songPath);
            //printf("MD5 values match. Expected: %s, \nCalculated: %s\n and path %s", downloadInfo->md5sum, calculated_md5,downloadInfo->songPath);
        if (calculated_md5 != NULL && strcmp(downloadInfo->md5sum, calculated_md5) == 0) {
            write(1,"MD5 verification successful\n",sizeof("MD5 verification successful\n"));
                close(downloadInfo->fd_song);
        } else {
            write(1,"MD5 verification failed\n",sizeof("MD5 verification failed\n"));
                close(downloadInfo->fd_song);

        }
        free(calculated_md5);
    }

    return NULL;
}
void createBinaryFile(Frame *frame,FileInfo *fileInfo) {
    if (fillDownloadInfo(frame, fileInfo) != 0) {  
        perror("Error processing file info.\n");
        return;
    }
    fileInfo->songPath = malloc(strlen(fileInfo->fileName) + strlen(".mp3") + 1);
    sprintf(fileInfo->songPath, "%s.mp3", fileInfo->fileName);   // sprintf(fileInfo->songPath, "%s.mp3", fileInfo->fileName);
    fileInfo->fd_song = open(fileInfo->songPath, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fileInfo->fd_song == -1) {
        perror("Error opening file for download");
        free(fileInfo->fileName);
        free(fileInfo->md5sum);
        free(fileInfo->songPath);
    } 
}

void processFileResponse(FileInfo *fileInfo) {
       
    FileInfo *threadInfo = malloc(sizeof(FileInfo));
    *threadInfo = *fileInfo;
    //pthread_t downloadThread;
    if (pthread_create(&downloadThread, NULL, downloadSongs, threadInfo) != 0) {    // nou thread pels Downloads
        perror("Error creating download thread");
        free(threadInfo);
    }
}
void messageQueue(Frame *frame,int mq_id,int id_bustia) {
    MessageQueue msg;
    msg.frame = *frame;
    msg.mtype = id_bustia;
    //printf("Envio per la cua %d + id bustia : %ld \n\n", mq_id, msg.mtype);
    if (msgsnd(mq_id, &msg, sizeof(MessageQueue)- sizeof(long) , 0) == -1) {    //if (msgsnd(mq_id, &msg, sizeof(Frame) - sizeof(long), 0) == -1) {
        perror("Error al enviar el mensaje");
        exit(EXIT_FAILURE);
    }
    usleep(4000);
}

void *socketListener(void *arg) {
    ThreadArgs *args = ( ThreadArgs *)arg;
    if (args == NULL) {
        printF("Error: ThreadArgs is NULL\n");
        return NULL;  // or handle the error appropriately
    }
    int mq_id = args->mq_id;
    FileInfo *fileInfo = malloc(sizeof(FileInfo));
    fileInfo->id_queue = mq_id;
   // printf("Envio per la cua %d + id bustia :\n\n", mq_id );
    while (tocaTancar==1) {
        Frame frame;
       
        if (receive_frame(sockfd_poole, &frame) < 0 && tocaTancar==1) {
            printF("Error");
            break;
        }  
        if (strcmp(frame.header, "SONGS_RESPONSE") == 0) {
            processSongsResponse(&frame);
        } else if (strcmp(frame.header, "PLAYLISTS_RESPONSE") == 0) {
            processPlaylistsResponse(&frame);
        } else if (strcmp(frame.header, "NEW_FILE") == 0) {
            print_frame2(&frame);
            int id_bustia2 = extractIdFromFrame2(&frame);
            printf("id bustia fora NWE FILE : %d\n ",id_bustia2);
            fileInfo->id_bustia=id_bustia2;
            createBinaryFile(&frame, fileInfo); 

            processFileResponse(fileInfo); 
       }
        else if (strcmp(frame.header, "FILE_DATA") == 0) {

            int id_bustia = extractIdFromFrame(&frame);

            messageQueue(&frame,mq_id,id_bustia);            //implementem cua de missatges;
        }
        
        free(frame.header);
        free(frame.data);
    }
    free(fileInfo->fileName);
    free(fileInfo->md5sum);
    free(fileInfo->songPath);        
    free(fileInfo);
       
  
    return NULL;
}
int connectToPoole(char *tokens[]) {
    
    if (!tokens[1] || !tokens[2]) {
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
    if (strcmp(frameAcknowledge.header, "CON_OK") == 0) {        // LA Connexio amb Poole es bona, per tant, llancem thread per escoltar trames.
        
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
void listSongs(int *connectedOrNot) {
    if(*connectedOrNot){
        char frame_buffer[FRAME_SIZE] = {0};
        fillFrame(frame_buffer,0x02,"LIST_SONGS"," ");
        
        send(sockfd_poole, frame_buffer, FRAME_SIZE, 0);//Bowman send poole
                printf("SEND COMANDA: %s    AND NOW WE WAIT FOR RESPONE\n","LIST_SONGS");
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
    } else {
        printF("Not connected to HAL 9000\n");
    }
}
void logoutDiscovery(){
    char frame_buffer[FRAME_SIZE] = {0};
    fillFrame(frame_buffer,0x06,"EXIT",tokens[0]);
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
    close(sockfd);
}

void freeMemory(){   //lliberar memoria pero no entenc el numUsuaris??? TODO CANVIARHO
    int i;
    for(i=0; i<numUsuaris; i++){
        free(bowmaneta[i].fullName);
        free(bowmaneta[i].pathName);
        free(bowmaneta[i].ipDiscovery);
    }
    free(bowmaneta);
}

void logout(){
    pthread_cancel(listenerThread);
    pthread_cancel(downloadThread);

    logoutDiscovery(); //A tokens li envia el nom del servidor
    msgctl (mq_id_queue, IPC_RMID, (struct msqid_ds *)NULL);    //Que algun dels dos procesos elimini la bustia
    tocaTancar=0;


    char frame_buffer[FRAME_SIZE] = {0};
    fillFrame(frame_buffer,0x02,"EXIT",bowmaneta[0].fullName);
    freeMemory(); 

    send(sockfd_poole, frame_buffer, FRAME_SIZE, 0);//Bowman send poole

    char info[256];
    int errorSocketOrNot=read(sockfd_poole, info, 256);//bowman recibe from poole
    Frame frameAcknoledge;
    printaAcknowledge(info,&frameAcknoledge);
   // print_frame2(&frameAcknoledge);
    //printf("Error: %d",errorSocketOrNot);
    if(errorSocketOrNot!=-1 ){
        if(strcmp(frameAcknoledge.header,"CON_OK")){
            close(sockfd_poole);//Crec que no es tindra que fer perque sino es tanca la comunicacio
            printF("Thanks for using HAL 9000, see you soon, music lover!\n");
            exit(0);
        }
    }   

}

void kctrlc(){ 
    logout();
}

int controleCommands(char *whichCommand, int *connectedOrNot) {
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
            logout();
            return 0;
        }
        if(strcasecmp("LIST",whichCommand1) == 0){//TODO F2
            whichCommand2=strtok(NULL, &delimiter);//Si li fiquem NULL començara la segona busqueda per on es va quedar cuan es va cridar per primer cop strtok
            if(whichCommand2 != NULL){
                if(strcasecmp("SONGS",whichCommand2) == 0){
                    listSongs(connectedOrNot); 
                    flag=1;
                }
                else if(strcasecmp("PLAYLISTS",whichCommand2) == 0){
                    listPlaylists(connectedOrNot);
                    flag=1;
                }
            }
        }
        if(strcasecmp(whichCommand1,"DOWNLOAD") == 0){ //TODO F3
            download(connectedOrNot, whichCommand);
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
        
        else if(flag==0){
            printF("ERROR: Please input a valid command.\n");
        }
    }
    return 1;
}

int main(int argc, char *argv[]) {
    signal(SIGINT, kctrlc);
    //signal(SIGKILL, kctrlc);

    if (argc != 2) {
        printF("ERROR: Incorrect number of arguments\n");
        return -1;
    }
    bowmaneta = readTextFile(argv[1], &numUsuaris);
    if (bowmaneta == NULL) {   
        return -2;
    }
    char *command;
    int connectedOrNot = 0;
    key_t key = IPC_PRIVATE; // Use IPC_PRIVATE to generate a unique key
    if (key == -1) {
        perror("Error generating key");
        fprintf(stderr, "Additional information: Something went wrong with ftok.\n");
        //exit(EXIT_FAILURE);
    }
        mq_id_queue=msgget(key, IPC_CREAT | 0666);
    if (mq_id_queue == -1) {
        perror("Error al obtener/crear la cola de mensajes");
        printF("Error al crear la cola de mensajes\n");
        exit(EXIT_FAILURE);
    }

    write(1, "\n$", 3);
    while (tocaTancar==1) {
        command = read_until(STDIN_FILENO, '\n');         /* 1R THREAD ESCOLTAR DE LA TERMINAL/ //canvi fet que deien els becaris*/
        if (command == NULL || strlen(command) == 0) {// Leer el comando del usuario
            free(command); 
            continue; 
        }
        int newCommand=controleCommands(command, &connectedOrNot);

        if(newCommand==1){
            ThreadArgs *threadArgs = malloc(sizeof(ThreadArgs));
            if (threadArgs == NULL) { // Handle the case where malloc fails to allocate memory
                perror("Error allocating memory for threadArgs");
                exit(EXIT_FAILURE);
            }

            threadArgs->mq_id = mq_id_queue;
            //pthread_t listenerThread;

            if (pthread_create(&listenerThread, NULL, socketListener, threadArgs) != 0) {
                perror("Error al crear el hilo de escucha");
                exit(EXIT_FAILURE);

                close(sockfd_poole);
            }
        }

        write(1, "\n$", 3);
        free(command); 
    }
    msgctl (mq_id_queue, IPC_RMID, (struct msqid_ds *)NULL);    //Que algun dels dos procesos elimini la bustia
    return 0;
}