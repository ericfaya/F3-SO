#include "Bowman.h"
#include "config.h"

Bowman* bowmaneta;
int numUsuaris;
char *tokens[MAX_TOKENS];
int sockfd_poole;
int isConnectedToPoole = 0;

pthread_mutex_t socket_mutex = PTHREAD_MUTEX_INITIALIZER;

int fillDownloadInfo(const Frame *file_info_frame, FileInfo *downloadInfo) {
   // printf("Iniciando fillDownloadInfo...\n");
    char *token, *dataCopy;

    dataCopy = strdup(file_info_frame->data);

    if (dataCopy == NULL) {
        perror("strdup failed");
        return -1;
    }
//    printf("Data copiada: %s\n", dataCopy);

    token = strtok(dataCopy, "&");    // Extracción de FileName
    if (token == NULL) {
        free(dataCopy);
        return -1;
    }
    //printf("FileName extraído: %s\n", token);
    downloadInfo->fileName = strdup(token);

    token = strtok(NULL, "&");    // Extracción de fileSize
    if (token == NULL) { 
        free(downloadInfo->fileName);
        free(dataCopy);
        return -1;
    }
   // printf("FileSize extraído: %s\n", token);
    downloadInfo->fileSize = atoi(token);

    token = strtok(NULL, "&");    // Extracción de md5sum
    if (token == NULL) {
        free(downloadInfo->fileName);
        free(dataCopy);
        return -1;
    }
    //printf("MD5SUM extraído: %s\n", token);
    downloadInfo->md5sum = strdup(token);

    token = strtok(NULL, "&");    // Extracción de songId
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

int receiveFileData(int sockfd, FileInfo *downloadInfo) {
    Frame incoming_frame;
    downloadInfo->totalBytesReceived = 0;

    //printf("Inicio de receiveFunctionFileData. fileSize esperado: %zd bytes\n", fileSize);
    key_t key = ftok("S10_cocinero.c", 1);

    //int mq_id = msgget (IPC_PRIVATE, 0600 | IPC_CREAT);//msgget(key, IPC_CREAT | 0666);
    int mq_id = msgget(key, IPC_CREAT | 0666);
     //   printf("The message q-id is: %d",mq_id);

    if (mq_id == -1) {
        printF("Error al crear la cola de mensajes\n");
        exit(EXIT_FAILURE);
    }

    MessageQueue msg;

    while (downloadInfo->totalBytesReceived < downloadInfo->fileSize-600) {
        //usleep(1000);
    
        pthread_mutex_lock(&socket_mutex);
        //msgrcv(mq_id, &msg, sizeof(msg) - sizeof(long), 1000, IPC_NOWAIT) aixi enteroia la mida es variable
        if(receive_frame(sockfd, &incoming_frame) >= 0) {
        //if (msgrcv(mq_id, &msg, 256*sizeof(char),1000, IPC_NOWAIT) != -1){/* ||  receive_frame(sockfd, &incoming_frame) >= 0) {   //Fiquem IPC_NOWAIT perque no sigui bloquejant*/
            pthread_mutex_unlock(&socket_mutex);

            if (strcmp(incoming_frame.header, "FILE_DATA") == 0) {

                if(writeBinaryFile(incoming_frame,downloadInfo)==-1)
                    return -1; //Error al escriure al file binary
                
                //printf("Received and wrote %zd bytes of data in this frame, total data received: %zd bytes\n", bytes_written, downloadInfo->totalBytesReceived);
            } 

            free(incoming_frame.header);
            free(incoming_frame.data);
        }
        
        
        else if(msgrcv(mq_id, &msg, sizeof(msg) - sizeof(long)/*256*sizeof(char )*/, 0, IPC_NOWAIT)>=0){// aixi enteroia la mida es variable) if (msgrcv(mq_id, &msg, 256*sizeof(char),1000, IPC_NOWAIT) != -1){
            if (strcmp(msg.frame.header, "FILE_DATA") == 0) {
                print_frame5(&msg.frame);
                if(writeBinaryFile(msg.frame,downloadInfo)==-1)
                    return -1; //Error al escriure al file binary
                //printf("Received and wrote %zd bytes of data in this frame, total data received: %zd bytes\n", bytes_written, downloadInfo->totalBytesReceived);
            } 

            //free(msg.frame.header);
           // free(msg.frame.data);
        }
        else{ 
            printF("Error receiving frame");
            perror("Error receiving frame");
            break;

        }
    }

    printf("Total data received: %zd bytes\n", downloadInfo->totalBytesReceived);
    
    return (downloadInfo->totalBytesReceived == downloadInfo->fileSize) ? 0 : -1;
}


void *downloadSongs(void *arg) {
    FileInfo *downloadInfo = (FileInfo *)arg;
    //printf("\nIniciando download Songs. Descargando: %s, fileSize: %d\n", downloadInfo->fileName, downloadInfo->fileSize);
   
    if (receiveFileData(sockfd_poole, downloadInfo) == 0){
        //printf("Download completed\n");  

        char *calculated_md5 = calculateMD5(downloadInfo->songPath);
        if (calculated_md5 != NULL && strcmp(downloadInfo->md5sum, calculated_md5) == 0) {
            write(1,"MD5 verification successful\n",sizeof("MD5 verification successful\n"));
        } else {
            write(1,"MD5 verification failed\n",sizeof("MD5 verification failed\n"));
        }
        free(calculated_md5);
    } else {
        printF("Download failed\n");  
    }
   
    close(downloadInfo->fd_song);

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
        //pthread_exit(NULL);
       // return NULL;
    } 
}

void processFileResponse(FileInfo *fileInfo) {
       
    FileInfo *threadInfo = malloc(sizeof(FileInfo));
    *threadInfo = *fileInfo;
    pthread_t downloadThread;
    if (pthread_create(&downloadThread, NULL, downloadSongs, threadInfo) != 0) {    // nou thread pels Downloads
        perror("Error creating download thread");
        free(threadInfo);
    }
    //pthread_join(downloadThread,NULL);
}

void messageQueue(Frame *frame, MessageQueue *msg,int mq_id) {
    msg->frame=*frame;
    print_frame4(&msg->frame);
    msg->mtype=1; //Long,a quina bustia enviarem el misssatge Sempre utilitzem la variable M1 per enviar el missatge,definim el id del missatge com  a 1
    printF("Enviant frames de la message queue");
   if (msgsnd(mq_id, &msg, sizeof(msg) - sizeof(long), 0) == -1) {// if (msgsnd(mq_id, &msg, 256*sizeof(char), 0) == -1) {// //Enviem 256 bytes(la trama),crec que no cal contar el long
        printF("Error al enviar el mensaje\n");
            perror("Error en msgsnd");
if (errno == EAGAIN) {
        // La cola de mensajes está llena y IPC_NOWAIT está establecido
        // Puedes tomar medidas específicas para manejar este caso
        printf("La cola de mensajes está llena. Mensaje no enviado.\n");
    } else {
        // Otro tipo de error
        printf("Error desconocido al enviar el mensaje.\n");
    }
        exit(EXIT_FAILURE);
    }
}

void *socketListener() {

   key_t key = ftok("S10_cocinero.c", 1);
   int mq_id=msgget(key, IPC_CREAT | 0666);
    //int mq_id = msgget (IPC_PRIVATE, 0600 | IPC_CREAT);//msgget(key, IPC_CREAT | 0666);
    printf("The message q-id is: %d",mq_id);
    if (mq_id == -1) {
        printF("Error al crear la cola de mensajes\n");
        exit(EXIT_FAILURE);
    }

    MessageQueue msg;
    
    FileInfo *fileInfo = malloc(sizeof(FileInfo));

    while (1) {
        Frame frame;
    
        pthread_mutex_lock(&socket_mutex);
        if (receive_frame(sockfd_poole, &frame) < 0) {
            printF("Error");
            break;
        }  
        
        pthread_mutex_unlock(&socket_mutex); 

      //if (strcmp(frame.header, "FILE_DATA") != 0) 
        if (strcmp(frame.header, "SONGS_RESPONSE") == 0) {
            processSongsResponse(&frame);
        } else if (strcmp(frame.header, "PLAYLISTS_RESPONSE") == 0) {
            processPlaylistsResponse(&frame);
        } else if (strcmp(frame.header, "NEW_FILE") == 0) {
                              print_frame2(&frame);

            createBinaryFile(&frame, fileInfo);    
            processFileResponse(fileInfo);            
        }
        else if (strcmp(frame.header, "FILE_DATA") == 0) {
            //writeBinaryFile(frame,fileInfo);  
                 // print_frame2(&frame);
            //sleep(1);
              //  pthread_mutex_lock(&socket_mutex);
                //   pthread_mutex_unlock(&socket_mutex); 
 FileInfo *threadInfo = malloc(sizeof(FileInfo));
    *threadInfo = *fileInfo;
    pthread_t downloadThread;
    if (pthread_create(&downloadThread, NULL, downloadSongs, threadInfo) != 0) {    // nou thread pels Downloads
        perror("Error creating download thread");
        free(threadInfo);
    }
            messageQueue(&frame,&msg,mq_id);            //implementem cua de missatges;
                //   pthread_mutex_unlock(&socket_mutex); 

        }
        
        free(frame.header);
        free(frame.data);
    }
    free(fileInfo->fileName);
    free(fileInfo->md5sum);
    free(fileInfo->songPath);        
    free(fileInfo);
       
    msgctl (mq_id, IPC_RMID, (struct msqid_ds *)NULL);    //Que algun dels dos procesos elimini la bustia
  
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

void listSongs(int *connectedOrNot) {
    if(*connectedOrNot){
        char frame_buffer[FRAME_SIZE] = {0};
        fillFrame(frame_buffer,0x02,"LIST_SONGS"," ");
        
        send(sockfd_poole, frame_buffer, FRAME_SIZE, 0);//Bowman send poole
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

void logout(){
    logoutDiscovery(); //A tokens li envia el nom del servidor
    
    char frame_buffer[FRAME_SIZE] = {0};
    fillFrame(frame_buffer,0x02,"EXIT",bowmaneta[0].fullName);

    send(sockfd_poole, frame_buffer, FRAME_SIZE, 0);//Bowman send poole
    
    char info[256];
    int errorSocketOrNot=read(sockfd_poole, info, 256);//bowman recibe from poole
    Frame frameAcknoledge;
    printaAcknowledge(info,&frameAcknoledge);
   // char *header;
    if(errorSocketOrNot!=-1 ){
        if(strcmp(frameAcknoledge.header,"[CON_OK]")){
            close(sockfd_poole);//Crec que no es tindra que fer perque sino es tanca la comunicacio
            printF("Thanks for using HAL 9000, see you soon, music lover!\n");
            exit(0);
        }//ELSE //  header = "CON_KO";
    }   
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

void freeMemory(Bowman* bowmaneta, int numUsuaris){   //lliberar memoria pero no entenc el numUsuaris??? TODO CANVIARHO

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
        printF("ERROR: Incorrect number of arguments\n");
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
       
        command = read_until(STDIN_FILENO, '\n');         /* 1R THREAD ESCOLTAR DE LA TERMINAL/ //canvi fet que deien els becaris*/
        if (command == NULL || strlen(command) == 0) {// Leer el comando del usuario
            free(command); 
            continue; 
        }
        controleCommands(command, &connectedOrNot);
        write(1, "\n$", 3);
        free(command); 
    }

    return 0;
}