#include "Bowman.h"
#include "config.h"

Bowman* bowmaneta;
int numUsuaris;
char *tokens[MAX_TOKENS];
int sockfd_poole;

char *extractMD5SUM(const char *file_info) {
    char *file_info_copy = strdup(file_info); // Hacer una copia ya que strtok modifica la cadena original
    if (file_info_copy == NULL) {
        perror("strdup failed");
        return NULL;
    }

    char *token;
    char *md5sum = NULL;
    token = strtok(file_info_copy, "&");
    token = strtok(NULL, "&");
    token = strtok(NULL, "&");

    if (token != NULL) {
        md5sum = strdup(token);
    }

    free(file_info_copy);
    return md5sum; 
}

char *calculateMD5(const char *filename) {
    char *command = (char *)malloc(strlen(filename) + 10);
    sprintf(command, "md5sum \"%s\"", filename);//el filename es el .mp3
//todo cambiar 
    FILE *stream = popen(command, "r"); // executem comanda en mode lectura desde aqui 
    free(command);

    if (stream == NULL) {
        perror("popen");
        return NULL;
    }

    char *md5sum = (char *)malloc(33); // es un hash de 32 caracters mes \0
    if (fscanf(stream, "%32s", md5sum) != 1) {
        perror("fscanf");
        free(md5sum);
        pclose(stream);
        return NULL;
    }

    pclose(stream);
    return md5sum; //shaura d lliberar memoria
}

int verifyMD5SUM(const char *file_path, const char *expected_md5) {
    char *actual_md5 = calculateMD5(file_path);
    if (actual_md5 == NULL) {
        return 1; 
    }

    int result = strcmp(actual_md5, expected_md5);
    free(actual_md5);

    return (result == 0) ? 0 : 1; //0 iguals, 1 diferents
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

void connectToPoole(char *tokens[]) {
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
    
    send(sockfd_poole, frame_buffer, FRAME_SIZE, 0);//Bowman send poole
    char info[256];
    read(sockfd_poole, info, 256);//bowman recibe from poole
    Frame frameAcknoledge;
    printaAcknowledge(info,&frameAcknoledge);

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

void printInfo(Bowman* bowmaneta){
    int i;
    char *buffer;
    for(i=0;i<1;i++){
        asprintf(&buffer,"%s user initialized\n", bowmaneta[i].fullName);  
        write(STDOUT_FILENO, buffer, strlen(buffer));   
        free(buffer);   
    }
}

int connectBowman(char *tokens[MAX_TOKENS]){ 
    connectDiscovery(tokens);
    printF(" connected to HAL 9000 system, welcome music lover!\n");
    connectToPoole(tokens);
      
    return 1; 
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


int receiveFileData(int sockfd, int fd_song) {
    Frame incoming_frame;
    int fileCompletat = 0;
  char *tokens[MAX_TOKENS];
                splitFrame(&incoming_frame,tokens);
                for (int i = 0; tokens[i] != NULL; ++i) {
                    printf("Token %d: %s\n", i + 1, tokens[i]);
                }
    while (!fileCompletat) {
        int errorSocketOrNot = receive_frame(sockfd, &incoming_frame);
        if (errorSocketOrNot >= 0) {
            if (strcmp(incoming_frame.header, "FILE_DATA") == 0) {
                size_t data_length = FRAME_SIZE - 3 - incoming_frame.header_length;
              
                ssize_t bytes_written = write(fd_song, incoming_frame.data, data_length);
                
                if (bytes_written == -1) {
                    perror("Error en la escritura del archivo");
                    free(incoming_frame.header);
                    free(incoming_frame.data);
                    return -1;
                }
            } else if (strcmp(incoming_frame.header, "FILE_END") == 0){
                fileCompletat = 1;
            }

            free(incoming_frame.header);
            free(incoming_frame.data);
        } else {
            return -1;
        }
    }
    return 0;
}

void *downloadSongs  (void *arg){
    
    int fd_song = *((int *)arg);  // Cast and dereference the argument
    if (receiveFileData(sockfd_poole, fd_song) == 0) {
        //printF("Descarga completada\n");
    }
    close(fd_song);

    pthread_exit(NULL); // Terminar el hilo

    return NULL;
}

void download(int *connectedOrNot, char *commandInput){
    printF("Download started!\n");
    if (*connectedOrNot == 1) {
        char frame_buffer[FRAME_SIZE] = {0};
        char *command = malloc(strlen(commandInput) + 1);

        strcpy(command, commandInput);
        char *song_name = strtok(NULL, "");

        if (song_name != NULL) {
            fillFrame(frame_buffer, 0x03, "DOWNLOAD_SONG", song_name); 
        } else {
            printF("No song name provided\n");
            free(command); 
            return;
        }
        send(sockfd_poole, frame_buffer, FRAME_SIZE, 0);
        free(command);

        // Recibir el frame de información del archivo
        Frame file_info_frame;
        receive_frame(sockfd_poole, &file_info_frame);
        
        char song_path[PATH_MAX];
        sprintf(song_path, "%s.mp3", song_name);
        int fd_song = open(song_path, O_WRONLY | O_CREAT | O_TRUNC, 0666);


        printF("Descarga completada\n");

        pthread_t t1;
        //void *res;
        int s;
        s = pthread_create (&t1, NULL, downloadSongs, (void *)&fd_song); //TODO ENVIAR MD6SUM
        if (s != 0){
            printF("pthread_create\n");
            exit (EXIT_FAILURE);
        } 
       /* s = pthread_join (t1, &res); //TODO no ca esperarse el join,tu lo que vols es que es fagi en paralel
        if (s != 0){
            printf("pthread_join\n");
            exit (EXIT_FAILURE);
        }*/
        

       /* if (receiveFileData(sockfd_poole, fd_song) == 0) {AIXO ho farem al thread
            printF("Descarga completada\n");
        }
        close(fd_song);*/

        // Verificar MD5SUM
        char *original_md5 = extractMD5SUM(file_info_frame.data);
        if (verifyMD5SUM(song_path, original_md5) == 0) {
            printF("Verificación MD5 exitosa\n");
        } else {
            printF("Error en la verificación MD5\n");
        }

        free(file_info_frame.header);
        free(file_info_frame.data);
    } else {
        printF("Cannot download, you are not connected to HAL 9000\n");
    }
}

void listSongs(int *connectedOrNot){//TODO F2
    if(*connectedOrNot){
        char frame_buffer[FRAME_SIZE] = {0};
        fillFrame(frame_buffer,0x02,"LIST_SONGS"," ");
        
        send(sockfd_poole, frame_buffer, FRAME_SIZE, 0);//Bowman send poole

        Frame incoming_frame;
        
        int errorSocketOrNot = receive_frame(sockfd_poole, &incoming_frame);
        
        if (errorSocketOrNot >= 0) {
            //TODO mostrar les cansons
        } else {
            perror("Error\n");
        }
        
        free(incoming_frame.header);
        free(incoming_frame.data);
    }
    else{
        printF("Cannot list, you are not connected to HAL 9000\n");
    } 
}

void listPlaylists(int *connectedOrNot){//TODO F2
     if(*connectedOrNot){
        char frame_buffer[FRAME_SIZE] = {0};
        fillFrame(frame_buffer,0x06,"LIST_PLAYLISTS"," ");

        send(sockfd_poole, frame_buffer, FRAME_SIZE, 0);//Bowman send poole

        Frame incoming_frame;
        int errorSocketOrNot = receive_frame(sockfd_poole, &incoming_frame);
        if (errorSocketOrNot >= 0) {
            
        } else {
            perror("Error\n");
        }
        
        free(incoming_frame.header);
        free(incoming_frame.data);
    }
    else{
        printF("Cannot list, you are not connected to HAL 9000\n");
    }
}

void checkDownload(int *connectedOrNot){
    if(*connectedOrNot){

    }
    else{
        printF("Cannot check, you are not connected to HAL 9000\n");
    }
}

void clearDownload(int *connectedOrNot){
    if(*connectedOrNot)
        printF("Clear\n");
    else
        printF("Cannot clear, you are not connected to HAL 9000\n");
}

int controleCommands(char whichCommand[50],int *connectedOrNot) {
    int flag=0;
    char *whichCommand1,*whichCommand2;
    const char delimiter = ' ';
    char *tokens[MAX_TOKENS];
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
            checkDownload(connectedOrNot);
            flag=1;
        }

        if(strcasecmp("CLEAR",whichCommand1) == 0){//TODO F3
            clearDownload(connectedOrNot);
            flag=1;
        }
        

        else if(flag==0){
            printF("ERROR: Please input a valid command.\n");
        }
    }
    return 1;
}

void kctrlc(){ 
    freeMemory(bowmaneta, numUsuaris);
    logout(); //SIGNAL CONTROL+C TODO: S'HAURA DE FER VARIABLE GLOBAL  crec EL FD SOCKET :int sockfd_poole 
}

int main(int argc, char *argv[]){
    char whichCommand[50];
    int connectedOrNot=0;    
    
    if (argc != 2){    //Llegir config.dat
        printF("ERROR: Incorrect number of arguments\n");
        return -1;
    }
                
    bowmaneta = readTextFile(argv[1], &numUsuaris);
    if (bowmaneta == NULL){   
        return -2;
    }

    printInfo(bowmaneta);
    signal(SIGINT, kctrlc);

    int exitOrNot=1;
     // int sockfd_poole=0;
    while(exitOrNot==1){   
        int bytesLlegits;

        write(1, "\n$", 3);
        bytesLlegits = read(0, whichCommand, 100);
        whichCommand[bytesLlegits-1] = '\0';
            
        exitOrNot=controleCommands(whichCommand,&connectedOrNot);
    }
    
    //freeAndClose(poole_frame,sockfd,poolete,numUsuaris);
    freeMemory(bowmaneta,numUsuaris); 
    return 0;   
}