#include "Bowman.h"
#include "config.h"

Bowman* bowmaneta;
int numUsuaris;
char *tokens[MAX_TOKENS];
int sockfd_poole;

void createFileSong(Frame *frame) {
    const char *file_path="Files/floyd/Lista2/Song1.mp3";
     int fd_file;
    char *buffer;
    asprintf(&buffer," %s:\n\n", file_path);  
    write(1, buffer, strlen(buffer));
    free(buffer);
    fd_file = open(file_path, O_RDWR | O_APPEND | O_CREAT , 0666);
    if(fd_file == -1){
        printF("ERROR: File not found\n");
    }else{
    
        ssize_t writeSize = write(fd_file, frame->data, strlen(frame->data));
         if (writeSize == -1) {
            perror("Error writing to file");
        } else {
            printf("Successfully wrote %zu bytes to the file\n", writeSize);
        }

        close(fd_file);
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

void *downloadSongs  (void *arg){
    
    char *data = (char *)arg;
    int contador=0;
    printf("data: %s",data);
    while(contador<10){//50 segfundos
        int multipli=5*contador;
        printf("\nHan pasado %d segundpos \n",multipli);
        sleep(5);
        contador++;
    }     
    pthread_exit(NULL); // Terminar el hilo

    return (void *) arg;
}

int receiveFileData(int sockfd, int fd_song) {
    Frame incoming_frame;
    int fileCompletat = 0;

    while (!fileCompletat) {
        int errorSocketOrNot = receive_frame(sockfd, &incoming_frame);
        if (errorSocketOrNot >= 0) {
            size_t data_capacity = FRAME_SIZE - 3 - incoming_frame.header_length;

            if (strcmp(incoming_frame.header, "FILE_DATA") == 0) {
                print_frame(&incoming_frame);
                ssize_t bytes_written = write(fd_song, incoming_frame.data, data_capacity/*strlen(incoming_frame.data)*/ /*244*/);
                //           printf("Total bytes written: %zd\n", bytes_written);

                if (bytes_written == -1) {
                    perror("Error");
                    free(incoming_frame.header);
                    free(incoming_frame.data);
                    return -1;
                }
               /* for (size_t i = 0; i < strlen(incoming_frame.data); i++) {
                printf("%c", incoming_frame.data[i]);
            } */
            } else {
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

void download(int *connectedOrNot, char *commandInput){
    printF("Download started!\n");
    if(*connectedOrNot==1){
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

        Frame incoming_frame;
        receive_frame(sockfd_poole, &incoming_frame);
        
        char song_path[PATH_MAX];
        sprintf(song_path, "%s.mp3", song_name);
         
        int fd_song = open(song_path, O_WRONLY | O_CREAT | O_TRUNC, 0666); //int fd_song = open(song_path, O_WRONLY | O_APPEND | O_CREAT, 0666);

        if (receiveFileData(sockfd_poole, fd_song) == 0) {
            printF("sha fet tota la descarga\n");
        }
        
        free(incoming_frame.header);
        free(incoming_frame.data);
       

        
    }
    else
        printF("Cannot download, you are not connected to HAL 9000\n");
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