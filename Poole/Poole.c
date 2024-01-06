#include "Poole.h"
#include "config.h"

Poole *poolete;
int numUsuaris;//Hem de saber cuantradas usuaris estan conectats en el servidor(discovery)
int sockfd_poole_server;
int tocaTancar=1;
Frame incoming_frame;
pthread_t thread_id;
int downloadFinshed=0;
size_t playlistFinshed=0;


pthread_mutex_t clientrada_sockets_mutex = PTHREAD_MUTEX_INITIALIZER; // Mutex per larray
ClientNode* head = NULL;
   
void removeAllClients() {
    pthread_mutex_lock(&clientrada_sockets_mutex);

    ClientNode* current = head;
    ClientNode* next;

    while (current != NULL) {
        next = current->next;
        close(current->sockfd); // Close the socket
        free(current);
        current = next;
    }

    head = NULL;

    pthread_mutex_unlock(&clientrada_sockets_mutex);
}
void kctrlc(){ 
    tocaTancar = 0;

    //pthread_cancel(fileTransferThread);
    //pthread_join(fileTransferThread, NULL);
    //pthread_detach(fileTransferThread);

    pthread_cancel(thread_id);
    pthread_join(thread_id, NULL);
    pthread_detach(thread_id);

    

    pthread_mutex_destroy(&clientrada_sockets_mutex);
    
    removeAllClients();
    close(sockfd_poole_server);
    for (int i = 0; i < numUsuaris; ++i){
        free(poolete[i].fullName);
        free(poolete[i].pathName);
        free(poolete[i].ipDiscovery);
        free(poolete[i].ipPoole);
    }

    free(poolete);
   // free(incoming_frame.header);
    //free(incoming_frame.data);
    printF("Thanks for using HAL 9000, see you soon, music lover!\n");

    exit(0);
}
void addClient(int sockfd) {
    ClientNode* newNode = (ClientNode*)malloc(sizeof(ClientNode));
    if (!newNode) {
        perror("malloc failed");
        exit(EXIT_FAILURE);
    }

    newNode->sockfd = sockfd;
    newNode->next = NULL;

    pthread_mutex_lock(&clientrada_sockets_mutex);

    if (head == NULL) {
        head = newNode;
    } else {
        ClientNode* current = head;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = newNode;
    }

    pthread_mutex_unlock(&clientrada_sockets_mutex);
}



void sendSongListResponse(int socket) {
    char data2[FRAME_SIZE - 3 - strlen("SONGS_RESPONSE")]; // -3 por 'type' y 'header_length'.
    char *songs = (char *)malloc(1024);
    listAllSongs("Files/floyd", songs);
    snprintf(data2, sizeof(data2), "%s", songs);
    char frame_buffer[FRAME_SIZE] = {0};
    fillFrame(frame_buffer,0x02,"SONGS_RESPONSE",data2);
    send(socket, frame_buffer, FRAME_SIZE, 0);//Bowman send poole
    free(songs);
}

void sendPlayListResponse(int socket) {
    char data2[FRAME_SIZE - 3 - strlen("PLAYLISTS_RESPONSE")]; 
    char *songs = (char *)malloc(1024);
    
    listPlayLists("Files/floyd", songs);
    
    snprintf(data2, sizeof(data2), "%s", songs);
    
    char frame_buffer[FRAME_SIZE] = {0};
    fillFrame(frame_buffer,0x02,"PLAYLISTS_RESPONSE",data2);
    send(socket, frame_buffer, FRAME_SIZE, 0);//Bowman send poole
    free(songs);
}

void *sendFileData(void *arg) {
    FileTransferInfo *info = (FileTransferInfo *)arg;

    struct stat st;

    if (stat(info->filePath, &st) == -1) {
        perror("Error al obtener información del archivo");
        return NULL;//return -1;
    }
    int file_size = st.st_size;
    char *md5sum = calculateMD5(info->filePath);        // Calcular el MD5SUM
    if (md5sum == NULL) {
        return NULL;//return -1;
    }
    int data_info_size = strlen(info->song_name) + 20 + 32 + 2;         // calculem tamany de data amb els 3 components
    char *data_info = malloc(data_info_size);
    if (data_info == NULL) {
        perror("No se pudo asignar memoria para data_info");
        free(md5sum);
        return NULL;//return -1;
    }
    snprintf(data_info, data_info_size, "%s&%d&%s&%d",info->song_name , file_size, md5sum,info->id);        //trama pel primer frame, el del md5
    char frame_buffer[FRAME_SIZE];        // Enviar la trama
    fillFrame(frame_buffer, 0x04, "NEW_FILE", data_info);
    send(info->socket, frame_buffer, FRAME_SIZE, 0); //aquest l'envia bé

    free(data_info); 
    free(md5sum);   

    int fd_file = open(info->filePath, O_RDONLY);
    if (fd_file == -1) {
        perror("Error opening file");
        free(info);
        return NULL;
    }
    char *header = "FILE_DATA";
    size_t header_len = strlen(header);
    size_t data_capacity = FRAME_SIZE - 3 - header_len - sizeof(int) - 1; // Espacio para ID y '&'
    char *buffer = (char *)malloc(data_capacity);
    if (!buffer) {
        perror("Failed to allocate memory for buffer");
        close(fd_file);
        free(info);
        return NULL;
    }
    ssize_t totalBytesSent = 0;
    ssize_t readSize;
    while ((readSize = read(fd_file, buffer, data_capacity)) > 0 && tocaTancar==1) {

        char frame_buffer[FRAME_SIZE] = {0};

        *(int *)(frame_buffer + 3 + header_len) = info->id;
        frame_buffer[3 + header_len + sizeof(int)] = '&';
        memcpy(frame_buffer + 3 + header_len + sizeof(int) + 1, buffer, readSize);
      
        ssize_t frameDataSize = readSize + sizeof(int) + 1; // Tamaño de los datos en el frame
        fillFrame2(frame_buffer, 0x04, header, frame_buffer + 3 + header_len, frameDataSize);

        send(info->socket, frame_buffer, FRAME_SIZE, 0);
        usleep(1000);

        //printf("Received and wrote %zd bytes of data in this frame, total data received: %zd bytes\n", readSize, totalBytesSent);
        totalBytesSent += readSize;
    }
    if (readSize == -1) {
        perror("Error reading from the file");
    }
    free(buffer);
    close(fd_file);
    free(info->song_name);
    free(info->filePath);

    // Free the memory for transferInfo
    free(info);
    pthread_mutex_lock(&clientrada_sockets_mutex);
    if(strcmp("DOWNLOAD_LIST",info->header) == 0){//TODO F3
        playlistFinshed++;
    }
    else if(strcmp("DOWNLOAD_SONG",info->header) == 0){//TODO F3
        downloadFinshed++;//Avisar de que ha acabat una canço
    }
    pthread_mutex_unlock(&clientrada_sockets_mutex);

    printF("BYEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE THREAD DOWNLOAD Songs JAMBELE\n\n");

    pthread_cancel(pthread_self());
    pthread_detach(pthread_self());

    return NULL;
}

void enviarAcknowledge(int newsock,ssize_t bytes_read/*,int errorSocketOrNot*/){
    char *header;
    if(bytes_read==-1 ){
        header = "[CON_KO]";
    }
    else{
        header = "CON_OK";
    }
    
    char frame_buffer[FRAME_SIZE] = {0};
    fillFrame(frame_buffer,0x01,header," ");
    send(newsock, frame_buffer, 256, 0);//Bowman send poole
}

    FileTransferInfo *initializeFileTransferInfo(const char *filePath, const char *songName, int socket, int id,const char *header) {
    FileTransferInfo *info = malloc(sizeof(FileTransferInfo));
    if (info == NULL) {
        perror("Error allocating memory for file transfer info");
        return NULL;
    }

    info->filePath = strdup(filePath);
    info->song_name = strdup(songName);
    info->header = strdup(header);


    if (info->filePath == NULL || info->song_name == NULL || info->header == NULL) {
        perror("Error allocating memory for filePath or song_name");
        free(info->filePath);
        free(info->song_name);
        free(info->header);

        free(info);
        return NULL;
    }
    info->socket = socket;
    info->id = id;

    return info;
}

int downloadSong(int socket,char *path_found,char *song_name,const char *header) {

    if (path_found != NULL) {
        int idNumRandom = 0 + rand() % 999;

        FileTransferInfo *transferInfo = initializeFileTransferInfo(path_found, song_name, socket, idNumRandom,header);
        if (transferInfo == NULL) {
            // Handle error
            perror("Error allocating memory for file transfer info");

            free(path_found);
            return -1;
        }
       
        pthread_t fileTransferThread;

        if (pthread_create(&fileTransferThread, NULL, sendFileData, transferInfo) != 0) {
            perror("Error creating file transfer thread");
            // Manejar error
            free(transferInfo->filePath);
            free(transferInfo->song_name);
            free(transferInfo->header);
            free(transferInfo);
            free(path_found);
            return -1;
        }

        //free(path_found);

        //free(song_name);
        
    } 
    return 0;
}

int handleBowmanConnection(int *newsock,ssize_t bytes_read/*, int errorSocketOrNot*/, Frame *incoming_frame) {
  
    if (strcmp(incoming_frame->header, "NEW_BOWMAN") == 0) { 
        char *buffer;
        asprintf(&buffer,"New user connected: %s.\n\n...", incoming_frame->data);  
        write(STDOUT_FILENO, buffer, strlen(buffer));   
        free(buffer);
    
        enviarAcknowledge(*newsock,bytes_read/*, errorSocketOrNot*/);
    }
    else if (strcmp(incoming_frame->header, "LIST_SONGS") == 0)
    {
        char *buffer;
        asprintf(&buffer,"New request – %s requires the list of songs.\nSending song list to %s\n\n",incoming_frame->data, incoming_frame->data);  
        write(STDOUT_FILENO, buffer, strlen(buffer));   
        free(buffer);
        sendSongListResponse(*newsock);
    }
    else if (strcmp(incoming_frame->header, "LIST_PLAYLISTS") == 0)
    {
        char *buffer;
        asprintf(&buffer,"New request – %s requires the list of playlists.\nSending playlist list to %s\n\n",incoming_frame->data, incoming_frame->data);  
        write(STDOUT_FILENO, buffer, strlen(buffer));   
        free(buffer);
        sendPlayListResponse(*newsock);
    }
    
    else if (strcmp(incoming_frame->header, "DOWNLOAD_SONG") == 0) //TODO    A total of 2 songs will be sent. AQUEST PRINTF,SA DE CONTAR EL NUMERO DE CANSONS O ALGO AIXI K SENVIEN
    {
        char *path_found=NULL;//char path_found[PATH_MAX];
        char *song_name = incoming_frame->data; 
        char *buffer;

        asprintf(&buffer,"New request – %s wants to download %s\n Sending %s to %s\n\n", incoming_frame->data,song_name,song_name,incoming_frame->data);  
        write(STDOUT_FILENO, buffer, strlen(buffer));   
        free(buffer);

        //printf("'%s' song fucking name ",song_name);
        path_found = findSongInDirectory("Files/floyd", song_name);
        //printf("'%s' path fucking found ",path_found);
        downloadSong(*newsock,path_found,song_name,"DOWNLOAD_SONG");
        
     
         
    }
    else if (strcmp(incoming_frame->header, "DOWNLOAD_LIST") == 0) //TODO    A total of 2 songs will be sent. AQUEST PRINTF,SA DE CONTAR EL NUMERO DE CANSONS O ALGO AIXI K SENVIEN
    {
        char *path_prefix = "Files/floyd/";
        char *full_path = malloc(strlen(path_prefix) + strlen(incoming_frame->data) + 1);

        if (full_path != NULL) {
            strcpy(full_path, path_prefix);
            strcat(full_path, incoming_frame->data);

            PathList resultList;
            initializePathList(&resultList, 1);

            findSongsInList(full_path, &resultList);
            for (size_t i = 0; i < resultList.size; ++i) {
                downloadSong(*newsock,resultList.paths[i],resultList.songs[i],"DOWNLOAD_LIST");
            }
            if(resultList.size == playlistFinshed){   
                pthread_mutex_lock(&clientrada_sockets_mutex);
                playlistFinshed = 0;//Avisar de que ha acabat una canço
                pthread_mutex_unlock(&clientrada_sockets_mutex);            
                char frame_buffer[FRAME_SIZE];        // Enviar la trama ACK INVENTADA
                fillFrame(frame_buffer, 0x08, "FINISH", "");
                send(*newsock, frame_buffer, FRAME_SIZE, 0); //aquest l'envia bé
            }
            freePathList(&resultList);
            free(full_path);
           
        }
        
       // return downloadSong(*newsock,incoming_frame);
    }
    
    else if (strcmp(incoming_frame->header, "CHECK_OK") == 0 || strcmp(incoming_frame->header, "CHECK_KO]") == 0)// NNNNNNNNNNNNNNNNNNNNNNEEEEEEEEEEEEEEEEEEEEEEEEWWWWWWWWWWWWWWWWWWWWWWW
    {
        //TODO,enviar un ack per que bowman sapiga que ha acabat?
        char *buffer;
        asprintf(&buffer,"Result MD5SUM– %s\n", incoming_frame->header);  
        write(STDOUT_FILENO, buffer, strlen(buffer));   
        free(buffer);    
           if(downloadFinshed != 0){   
            pthread_mutex_lock(&clientrada_sockets_mutex);
            downloadFinshed = 0;//Avisar de que ha acabat una canço
            pthread_mutex_unlock(&clientrada_sockets_mutex);            
            char frame_buffer[FRAME_SIZE];        // Enviar la trama ACK INVENTADA
            fillFrame(frame_buffer, 0x08, "FINISH", "");
            send(*newsock, frame_buffer, FRAME_SIZE, 0); //aquest l'envia bé
        }
        
    }
    else if (strcmp(incoming_frame->header, "EXIT") == 0)
    {
        enviarAcknowledge(*newsock,bytes_read/*,errorSocketOrNot*/);   
            
        return -1;
    }
    
    return 0;
}

void *clientHandler(void *args) {
    ThreadArgs *threadArgs = (ThreadArgs *)args;
    int clientSocket = threadArgs->socket;
    free(threadArgs);
    while (tocaTancar==1) {
       // Frame incoming_frame;
      
        ssize_t  bytes_read= receive_frame(clientSocket, &incoming_frame);
        if (bytes_read == -1) {
            printF("Error poole has been disconnected\n");
            kctrlc();
            break;        
        }
        else{
            int exitOrNot = handleBowmanConnection(&clientSocket,bytes_read/*, errorSocketOrNot*/, &incoming_frame);
            if (exitOrNot == -1) {
                free(incoming_frame.header);
                free(incoming_frame.data);
                break;
            }
           
        }
        free(incoming_frame.header);
        free(incoming_frame.data);
    }
    //close(clientSocket);
    printF("BYEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE THREAD CLIENT HANDLER\n\n"); //HO fa be

    pthread_cancel(pthread_self());
    pthread_detach(pthread_self());

    return NULL;
    //pthread_exit(NULL);
}
void connectToBowman(Poole *poolete) {
    uint16_t poole_port = poolete[0].portPoole;
    // Verifica que el puerto sea válido.
    if (poole_port < 1) {
        write(STDOUT_FILENO, "Error: Invalid port number(s)\n", sizeof("Error: Invalid port number(s)\n"));   
        exit(EXIT_FAILURE);
    }
    // Crea un socket para el servidor Poole.
    int sockfd_poole_server = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd_poole_server < 0) {
        perror("Error creating sockets");
        exit(EXIT_FAILURE);
    }
    // Configura la dirección y el puerto del servidor.
    struct sockaddr_in bowman_addr;
    memset(&bowman_addr, 0, sizeof(bowman_addr));
    bowman_addr.sin_family = AF_INET;
    bowman_addr.sin_addr.s_addr = INADDR_ANY;
    bowman_addr.sin_port = htons(poole_port);
    
    if (bind(sockfd_poole_server, (struct sockaddr *)&bowman_addr, sizeof(bowman_addr)) < 0) {
        perror("Error binding socket");
        exit(EXIT_FAILURE);
    }
    
    if (listen(sockfd_poole_server, 5) < 0) {
        perror("Error listening on socket");
        exit(EXIT_FAILURE);
    }
    while (tocaTancar==1) {
        struct sockaddr_in c_addr;
        socklen_t c_len = sizeof(c_addr);
        int newsock = accept(sockfd_poole_server, (struct sockaddr *)&c_addr, &c_len);
        if (newsock < 0) {
            perror("accept");
            return;  
        }
        addClient(newsock);

        ThreadArgs *args = malloc(sizeof(ThreadArgs));
        if (!args) {
            perror("Error al asignar memoria para args");
            close(newsock);
            return;
        }
        args->socket = newsock;
 
        //crear thread per gestionar connexio
        if (pthread_create(&thread_id, NULL, clientHandler, (void *)args) != 0) {
            perror("Error al crear thread");
            free(args);
            close(newsock);
        }
    }
    
    // Libera los recursos al final.
    close(sockfd_poole_server);
}

int main(int argc, char *argv[]){
    signal(SIGINT, kctrlc);
    //signal(SIGKILL, kctrlc);

    if (argc != 2){
        printF("ERROR: Incorrect number of argumentradas\n");
        return -1;
    }    
    
    poolete = readTextFile(argv[1], &numUsuaris);
    if (poolete == NULL){
        return -2;
    }
    char *userName2 = poolete[0].fullName;
    char *ipPoole = poolete[0].ipPoole;
    uint16_t portPoole = poolete[0].portPoole;
    char data2[FRAME_SIZE - 3 - strlen("NEW_POOLE")]; // -3 por 'type' y 'header_length'.
            memset(data2, 0, FRAME_SIZE - 3 - strlen("NEW_POOLE")); //INITIALIZE

    snprintf(data2, sizeof(data2), "%s&%s&%u", userName2, ipPoole, portPoole);
    char frame_buffer[FRAME_SIZE] ;
    memset(frame_buffer, 0, FRAME_SIZE); //INITIALIZE

    fillFrame(frame_buffer,0x01,"NEW_POOLE",data2);
    
    struct sockaddr_in server_addr;    //sockaddr_in: struct defineix l’estructura que permet configurar diversos paràmetres del socket com IP, port
    memset(&server_addr, 0, sizeof(server_addr));//Inicialitza,fica 0s a l'estructura
    server_addr.sin_family = AF_INET;//tipus de familia de socket es tracta
    server_addr.sin_port = htons(poolete[0].portDiscovery);//(Host To Network Short) Converteix port a big endian
    if (inet_pton(AF_INET, poolete[0].ipDiscovery, &server_addr.sin_addr) < 0) { //Converteix representradaació en text de la ip a l’equivalentrada binari (IPv4)
        perror("Invalid address/ Address not supported");
        exit(EXIT_FAILURE); 
        //fer free de memoria dinamica
    }
    int sockfd =0;
    sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sockfd < 0) {
        perror("Cannot create socket");
        exit(EXIT_FAILURE);
        //fer free de memoria dinamica
    }
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection Failed");
        exit(EXIT_FAILURE);
        //fer free de memoria dinamica
        close(sockfd); //Fas close per si acas
    }
    ssize_t bytes_sent = send(sockfd, frame_buffer, FRAME_SIZE, 0);
    if (bytes_sent < 0) {
        perror("Error sending data");
    } else {
        char info[256];
        //Frame frameAcknoledge;
        read(sockfd, info, 256);
        printaAcknowledge(info,&incoming_frame);
        free(incoming_frame.header);
        free(incoming_frame.data);
        
        close(sockfd);
        char *buffer;
        asprintf(&buffer,"\nReading configuration file\nConnecting %s Server to the system..\nConnected to HAL 9000 System, ready to listen to Bowmans petitions\n\nWaiting for connections...\n\n", userName2);  
        write(STDOUT_FILENO, buffer, strlen(buffer));   
        free(buffer);
        connectToBowman(poolete);
        //freeAndClose(/*poole_frame,*/poolete,numUsuaris);
    }
   
    return 0;  
}