#include "Poole.h"
#include "config.h"

Poole *poolete;
int numUsuaris;//Hem de saber cuantradas usuaris estan conectats en el servidor(discovery)
int sockfd_poole_server;
int max_sd ;
int clientrada_sockets[10];
fd_set master_set;


void sendSongListResponse(int socket) {
    char data2[FRAME_SIZE - 3 - strlen("SONGS_RESPONSE")]; // -3 por 'type' y 'header_length'.
    char *songs = (char *)malloc(1024);

    listAllSongs("Files/floyd", songs);

    snprintf(data2, sizeof(data2), "%s", songs);
    char frame_buffer[FRAME_SIZE] = {0};
    fillFrame(frame_buffer,0x02,"SONGS_RESPONSE",data2);

    send(socket, frame_buffer, FRAME_SIZE, 0);//Bowman send poole
}

void sendPlayListResponse(int socket) {
    char data2[FRAME_SIZE - 3 - strlen("PLAYLISTS_RESPONSE")]; 
    char *songs = (char *)malloc(1024);
    
    listPlayLists("Files/floyd", songs);
    
    snprintf(data2, sizeof(data2), "%s", songs);
    
    char frame_buffer[FRAME_SIZE] = {0};
    fillFrame(frame_buffer,0x02,"PLAYLISTS_RESPONSE",data2);

    send(socket, frame_buffer, FRAME_SIZE, 0);//Bowman send poole
}



void sendFileData(int socket, const char *file_path, int idNumRandom) {
    int fd_file = open(file_path, O_RDONLY);
    if (fd_file == -1) {
        perror("Error opening file");
        return;
    }

    char *header = "FILE_DATA";
    size_t header_len = strlen(header);
    size_t data_capacity = FRAME_SIZE - 3 - header_len - sizeof(int) - 1; // Espacio para ID y '&'

    char *buffer = (char *)malloc(data_capacity);
    if (!buffer) {
        perror("Failed to allocate memory for buffer");
        close(fd_file);
        return;
    }

    ssize_t totalBytesSent = 0;
    ssize_t readSize;
    while ((readSize = read(fd_file, buffer, data_capacity)) > 0) {
        char frame_buffer[FRAME_SIZE] = {0};

        // Preparar el frame con ID y datos
        *(int *)(frame_buffer + 3 + header_len) = idNumRandom;
        frame_buffer[3 + header_len + sizeof(int)] = '&';
        memcpy(frame_buffer + 3 + header_len + sizeof(int) + 1, buffer, readSize);

        // Enviar el frame
        ssize_t frameDataSize = readSize + sizeof(int) + 1; // Tamaño de los datos en el frame
        fillFrame2(frame_buffer, 0x04, header, frame_buffer + 3 + header_len, frameDataSize);
        send(socket, frame_buffer, FRAME_SIZE, 0);

        // Mostrar bytes de datos enviados
        totalBytesSent += readSize;
        //printf("Sent %zd bytes of data in this frame, total data sent: %zd bytes\n", readSize, totalBytesSent);
    }

    if (readSize == -1) {
        perror("Error reading from the file");
    }

    // Enviar el frame final para indicar el fin del archivo
   // char final_frame_buffer[FRAME_SIZE] = {0};
    //fillFrame2(final_frame_buffer, 0x04, "FILE_END", "", 0);
    //send(socket, final_frame_buffer, FRAME_SIZE, 0);

    free(buffer);
    close(fd_file);
}

int handleBowmanConnection(int *newsock,int errorSocketOrNot, Frame *incoming_frame) {
     if (errorSocketOrNot < 0) {
        perror("Error");//aqui s'hauria dafegir lo del KO
        close(*newsock);
    }
   
    if (strcmp(incoming_frame->header, "NEW_BOWMAN") == 0) { 
        char *buffer;
        asprintf(&buffer,"New user connected: %s.\n\n...", incoming_frame->data);  
        write(STDOUT_FILENO, buffer, strlen(buffer));   
        free(buffer);
    
        enviarAcknowledge(*newsock, errorSocketOrNot);
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
        char path_found[PATH_MAX];
        char *song_name = incoming_frame->data; 

        char *buffer;
        asprintf(&buffer,"New request – %s wants to download %s\n Sending %s to %s\n\n", incoming_frame->data,song_name,song_name,incoming_frame->data);  
        write(STDOUT_FILENO, buffer, strlen(buffer));   
        free(buffer);

        int found = findSongInDirectory("Files/floyd", song_name, path_found);
        if (found) {
            struct stat st;
            if (stat(path_found, &st) == -1) {
                perror("Error al obtener información del archivo");
                return -1;
            }
            int file_size = st.st_size;

            char *md5sum = calculateMD5(path_found);        // Calcular el MD5SUM
            if (md5sum == NULL) {
                return -1;
            }

            int data_info_size = strlen(song_name) + 20 + 32 + 2;         // calculem tamany de data amb els 3 components
            char *data_info = malloc(data_info_size);
            if (data_info == NULL) {
                perror("No se pudo asignar memoria para data_info");
                free(md5sum);
                return -1;
            }

            int idNumRandom = 0 + rand() % 999;
            snprintf(data_info, data_info_size, "%s&%d&%s&%d", song_name, file_size, md5sum,idNumRandom);        //trama pel primer frame, el del md5

            char frame_buffer[FRAME_SIZE];        // Enviar la trama

            fillFrame(frame_buffer, 0x04, "NEW_FILE", data_info);
            send(*newsock, frame_buffer, FRAME_SIZE, 0); //aquest l'envia bé
            
            sendFileData(*newsock, path_found,idNumRandom);

            free(data_info); 
            free(md5sum);    
        } 
    }
    

    else if (strcmp(incoming_frame->header, "EXIT") == 0)
    {
        enviarAcknowledge(*newsock,errorSocketOrNot);       
        return -1;
    }

    free(incoming_frame->header);
    free(incoming_frame->data);
    return 0;
}

void enviarAcknowledge(int newsock,int errorSocketOrNot) {
    char *header;
    if(errorSocketOrNot==-1 ){
        header = "[CON_KO]";
    }
    else{
        header = "CON_OK";
    }
    
    char frame_buffer[FRAME_SIZE] = {0};
    fillFrame(frame_buffer,0x01,header," ");
        
    send(newsock, frame_buffer, 256, 0);//Bowman send poole
}

void connectToBowman(Poole *poolete){
    uint16_t poole_port = poolete[0].portPoole;

    if (poole_port < 1) {
        write(STDOUT_FILENO, "Error: Invalid port number(s)\n", sizeof("Error: Invalid port number(s)\n"));   
        exit(EXIT_FAILURE);
    }
    sockfd_poole_server = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd_poole_server < 0) {
        perror("Error creating sockets");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in  bowman_addr;

    memset(&bowman_addr, 0, sizeof(bowman_addr));
    bowman_addr.sin_family = AF_INET;
    bowman_addr.sin_addr.s_addr = INADDR_ANY;
    bowman_addr.sin_port = htons(poole_port);

    if (bind(sockfd_poole_server, (struct sockaddr *)&bowman_addr, sizeof(bowman_addr)) < 0) { // enllaçar socketss a direccions
        perror("Error binding socket");
        exit(EXIT_FAILURE);
    }
    if (listen(sockfd_poole_server, 5) < 0) {    // Escoltar en el  socket ,escoltar bowmans
        perror("Error listening on socket");
        exit(EXIT_FAILURE);
    }
    /*fd_set master_set;*/
    FD_ZERO(&master_set);
    FD_SET(sockfd_poole_server, &master_set);
    /*int   pel sigfnal ho hem de fer global*/ max_sd = sockfd_poole_server;

/*  int clientrada_sockets[10];*/  
    for (int i = 0; i < 10; i++) {
        clientrada_sockets[i] = 0; 
    }
 
    while (1) {
        fd_set read_set = master_set;
        if (select(max_sd + 1, &read_set, NULL, NULL, NULL) < 0) {
            perror("select");
            exit(EXIT_FAILURE);
        }
        if (FD_ISSET(sockfd_poole_server, &read_set)) {        // proximes noves connexions
            struct sockaddr_in c_addr;
            socklen_t c_len = sizeof(c_addr);
            int newsock = accept(sockfd_poole_server, (struct sockaddr *)&c_addr, &c_len);
            if (newsock < 0) {
                perror("accept");
                exit(EXIT_FAILURE);
            }
            FD_SET(newsock, &master_set);

            for (int i = 0; i < max_sd; i++) {// afegir socket a l'array
                if (clientrada_sockets[i] == 0) {
                    clientrada_sockets[i]=newsock;  
                    if (newsock > max_sd) {
                        max_sd = newsock;
                    }
                    break;
                }
            }
        }

        for (int i = 0; i <= max_sd; ++i) {// sockets amb arrays ja afegits i creats
            int sd = clientrada_sockets[i];
            if (sd > 0 && FD_ISSET(sd, &read_set)) {
                Frame incoming_poole_frame;
                int errorSocketOrNot = receive_frame(sd, &incoming_poole_frame);
                int exitOrNot=handleBowmanConnection(&sd, errorSocketOrNot,&incoming_poole_frame);
                if (exitOrNot == -1) {
                    close(sd);
                    FD_CLR(sd, &master_set);
                    clientrada_sockets[i] = 0;
                }

                break;                
                if (errorSocketOrNot <= 0) {
                    close(sd);
                    FD_CLR(sd, &master_set);
                    clientrada_sockets[i] = 0;
                }
            }
        }
    }
    close(sockfd_poole_server);
}
    
void freeAndClose(Poole *poolete,int numUsuaris){ 
    int i;
    for(i=0; i<numUsuaris; i++){
        free(poolete[i].fullName);
        free(poolete[i].pathName);
        free(poolete[i].ipDiscovery);
        free(poolete[i].ipPoole);
    }
    free(poolete);
}

void kctrlc(){ 
    freeAndClose(/*poole_frame,*/poolete,numUsuaris);
    for (int i = 0; i <= max_sd; ++i) {// sockets amb arrays ja afegits i creats
        int sd = clientrada_sockets[i];
        close(sd);
        FD_CLR(sd, &master_set);
        clientrada_sockets[i] = 0;
    }
    close(sockfd_poole_server);

    //logout(); //SIGNAL CONTROL+C TODO: S'HAURA DE FER VARIABLE GLOBAL  crec EL FD SOCKET :int sockfd_poole 
}

int main(int argc, char *argv[]){
    //Poole *poolete;
    //int numUsuaris;//Hem de saber cuantradas usuaris estan conectats en el servidor(discovery)
    signal(SIGINT, kctrlc);

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
    snprintf(data2, sizeof(data2), "%s&%s&%u", userName2, ipPoole, portPoole);


    char frame_buffer[FRAME_SIZE] = {0};
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
    int sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
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

    send(sockfd, frame_buffer, FRAME_SIZE, 0);
    char info[256];
    Frame frameAcknoledge;

    read(sockfd, info, 256);
    printaAcknowledge(info,&frameAcknoledge);
    
    close(sockfd);
    char *buffer;
    asprintf(&buffer,"\nReading configuration file\nConnecting %s Server to the system..\nConnected to HAL 9000 System, ready to listen to Bowmans petitions\n\nWaiting for connections...\n\n", userName2);  
    write(STDOUT_FILENO, buffer, strlen(buffer));   
    free(buffer);
    

    connectToBowman(poolete);

    freeAndClose(/*poole_frame,*/poolete,numUsuaris);
    return 0;  
}