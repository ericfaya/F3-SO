#include "Poole.h"
#include "config.h"

void listSongsInDirectory(char *directory, char *result, int includeDirs) {
    DIR *directori;
    struct dirent *entrada;
    char path[1024];

    directori = opendir(directory);
    if (directori != NULL) {
        while ((entrada = readdir(directori)) != NULL) {
            if (entrada->d_name[0] == '.') {
                continue;
            }
            snprintf(path, sizeof(path), "%s/%s", directory, entrada->d_name);
            
            struct stat path_stat;
            stat(path, &path_stat);
            if (S_ISDIR(path_stat.st_mode)) {
                if (includeDirs) {
                    strcat(result, entrada->d_name);
                    strcat(result, "&");
                }
                listSongsInDirectory(path, result, includeDirs);
                if (includeDirs) {
                    strcat(result, "#");
                }
            } else if (strstr(entrada->d_name, ".mp3")) {
                size_t mida = strlen(entrada->d_name) - 4;
                //char nomSenseExt[mida];
                char nomSenseExt[mida + 1];
                strncpy(nomSenseExt, entrada->d_name, mida);
                nomSenseExt[mida] = '\0';
                strcat(result, nomSenseExt);
                strcat(result, "&");
            }
        }
        closedir(directori);
    } else {
        perror("No es pot obrir el directori");
    }
}

void listAllSongs(char *directory, char *result) {
    strcpy(result, "");
    listSongsInDirectory(directory, result, 0); // 0 per no ficar nom d llistes (directoris)
    size_t mida = strlen(result);
    if (mida > 0 && result[mida - 1] == '&') { //evitem un & o # al final
        result[mida - 1] = '\0';
    }
}

void listPlayLists(char *directory, char *result) {
    strcpy(result, "");
    listSongsInDirectory(directory, result, 1); // 1 per ficar noms d llistes
    size_t mida = strlen(result);

    while (mida > 0 && (result[mida - 1] == '#' || result[mida - 1] == '&')) { //evitem un & o # al final
        result[mida - 1] = '\0';
        mida--;
    }
}

void sendSongListResponse(int socket) {
    char data2[FRAME_SIZE - 3 - strlen("SONGS_RESPONSE")]; // -3 por 'type' y 'header_length'.
    char *songs = (char *)malloc(1024);

    listAllSongs("Files/floyd", songs);

    snprintf(data2, sizeof(data2), "%s", songs);
    char frame_buffer[FRAME_SIZE] = {0};
    doThingsTrama(frame_buffer,0x02,"SONGS_RESPONSE",data2);

    send(socket, frame_buffer, FRAME_SIZE, 0);//Bowman send poole
}

void sendPlayListResponse(int socket) {
    char data2[FRAME_SIZE - 3 - strlen("PLAYLISTS_RESPONSE")]; 
    char *songs = (char *)malloc(1024);
    
    listPlayLists("Files/floyd", songs);
    
    snprintf(data2, sizeof(data2), "%s", songs);
    
    char frame_buffer[FRAME_SIZE] = {0};
    doThingsTrama(frame_buffer,0x02,"PLAYLISTS_RESPONSE",data2);

    send(socket, frame_buffer, FRAME_SIZE, 0);//Bowman send poole
}

int handleBowmanConnection(int *newsock,int errorSocketOrNot, Frame *incoming_frame) {
     if (errorSocketOrNot < 0) {
        perror("Error");//aqui s'hauria dafegir lo del KO
        close(*newsock);
    }
   
    if (strcmp(incoming_frame->header, "NEW_BOWMAN") == 0) { 
        enviarAcknowledge(*newsock, errorSocketOrNot);
    }

    else if (strcmp(incoming_frame->header, "LIST_SONGS") == 0)
    {
        sendSongListResponse(*newsock);
    }

    else if (strcmp(incoming_frame->header, "LIST_PLAYLISTS") == 0)
    {
        sendPlayListResponse(*newsock);
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
    doThingsTrama(frame_buffer,0x01,header," ");
        
    send(newsock, frame_buffer, 256, 0);//Bowman send poole
}

void connectToBowman(Poole *poolete){
    char *buffer;
    asprintf(&buffer,"Connected to Discovery at %s:%d\n\n", poolete[0].ipDiscovery, poolete[0].portDiscovery);  
    write(STDOUT_FILENO, buffer, strlen(buffer));   
    free(buffer);
    uint16_t poole_port = poolete[0].portPoole;

    if (poole_port < 1) {
        write(STDOUT_FILENO, "Error: Invalid port number(s)\n", sizeof("Error: Invalid port number(s)\n"));   
        exit(EXIT_FAILURE);
    }
    int sockfd_poole_server = socket(AF_INET, SOCK_STREAM, 0);

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
    fd_set master_set;
    FD_ZERO(&master_set);
    FD_SET(sockfd_poole_server, &master_set);
    int max_sd = sockfd_poole_server;

    int clientrada_sockets[10];
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

int main(int argc, char *argv[]){
    Poole *poolete;
    int numUsuaris;//Hem de saber cuantradas usuaris estan conectats en el servidor(discovery)
    
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
    doThingsTrama(frame_buffer,0x01,"NEW_POOLE",data2);
    

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

    connectToBowman(poolete);

    freeAndClose(/*poole_frame,*/poolete,numUsuaris);
    return 0;  
}