#include "Discovery.h"
#include "config.h"
#include "PooleList.h"

Discovery *discovery;
int numUsuaris;
Frame incoming_poole_frame;
PooleList pooleList;
int sockfd_poole;
int sockfd_bowman;


void enviarAcknowledge(int newsock,int errorSocketOrNot,int bowmanOrPoole,PooleList *pooleList) {

    char *header=NULL;
    if(errorSocketOrNot==-1 ){
        header = "[CON_KO]";
    }
    else {
        header = "CON_OK";
    }
    
    char data2[FRAME_SIZE - 3 - strlen(header) ]; // -3 por 'type' y 'header_length'.
        memset(data2, 0, FRAME_SIZE - 3 - strlen(header)); //INITIALIZE

    if (bowmanOrPoole == 0 && pooleList->head != NULL) {
            PooleNode* pooleListMneysBalancejador = searchPooleListLessBowmans(pooleList);
            if (pooleListMneysBalancejador != NULL) {
                snprintf(data2, sizeof(data2), "%s&%s&%u", pooleListMneysBalancejador->info.userName, pooleListMneysBalancejador->info.ip, pooleListMneysBalancejador->info.port);
                pooleListMneysBalancejador->info.contador_bowmans++;
            } else {
                // No hay Pooles disponibles
                strcpy(data2, " "); // Envía datos vacíos
            }
        }
    else{ //Es 1 (poole)
        snprintf(data2, sizeof(data2), " ");
    }
    char frame_buffer[FRAME_SIZE] = {0};
    memset(frame_buffer, 0, FRAME_SIZE); //INITIALIZE

    fillFrame(frame_buffer,0x01,header,data2);

    write(newsock, frame_buffer, FRAME_SIZE);
    close(newsock);
}

void process_frame(Frame *frame, PooleList *list) {
    if (frame->type == 0x01 && strcmp(frame->header, "NEW_POOLE") == 0) {
        char *userName = NULL;
        char ip[INET_ADDRSTRLEN];
        int port;
        if (sscanf(frame->data, "%m[^&]&%15[^&]&%d", &userName, ip, &port) == 3) {
            if (userName != NULL) {   ///
                PooleInfo pooleInfo;
                pooleInfo.userName = strdup(userName);
                if (pooleInfo.userName == NULL) {
                    perror("Error allocating memory for userName");
                }
                else {
                    strcpy(pooleInfo.ip, ip);
                    pooleInfo.port = port;
                    pooleInfo.contador_bowmans= 0;
                    add_poole(list, pooleInfo);
                    // free(pooleInfo.userName);
                }
                               free(userName);  // Free the memory here

            }

            //free(userName);
        }
    }
}

void freeAndClose(){
    free_poole_list(&pooleList);
    close(sockfd_poole);
    close(sockfd_bowman);
}

void waitSocketPoole(int sockfd_poole,PooleList *pooleList){

    struct sockaddr_in c_addr;
    socklen_t c_len = sizeof(c_addr);
    int newsock = accept(sockfd_poole, (struct sockaddr *)&c_addr, &c_len);
    if (newsock < 0) {
        perror("accept");
         return;
    }
    
    int errorSocketOrNot=receive_frame(newsock, &incoming_poole_frame);
    process_frame(&incoming_poole_frame, pooleList);
    enviarAcknowledge(newsock,errorSocketOrNot,1,pooleList);
    printF("NEW_POOLE\n");
    free(incoming_poole_frame.header);
    free(incoming_poole_frame.data);
}

void waitSocketBowman(int sockfd_bowman,PooleList *pooleList){

    struct sockaddr_in c_addr;
    socklen_t c_len = sizeof(c_addr);
    int newsock = accept(sockfd_bowman, (struct sockaddr *)&c_addr, &c_len);
    if (newsock < 0) {
        perror("accept");
         return;
    }
     // Asegúrate de que la estructura Frame esté definida
    int errorSocketOrNot=receive_frame(newsock, &incoming_poole_frame);
    
    if (incoming_poole_frame.header == NULL) {
        perror("Error: Header not initialized");
        return;
    }
   

    if (strcmp(incoming_poole_frame.header, "EXIT") == 0){    
        removeBowmanFromPoole(pooleList,incoming_poole_frame.data); 
        return;       
    }
    free(incoming_poole_frame.header);
    free(incoming_poole_frame.data);
    enviarAcknowledge(newsock,errorSocketOrNot,0,pooleList);

    printF("NEW_BOWMAN\n");
}

void kctrlc(){ 
    freeAndClose();
    for (int i = 0; i < numUsuaris; ++i){
        free(discovery[i].ipPoole);
        free(discovery[i].ipBowman);
    }

    free(discovery);
    free(incoming_poole_frame.header);
    free(incoming_poole_frame.data);

    printF("Thanks for using HAL 9000, see you soon, music lover!\n");

    exit(0);
}

int main(int argc, char *argv[]) {
    signal(SIGINT, kctrlc);
    

    if (argc < 2) {
        printF("ERROR: Incorrect number of argumentradas\n");   
        exit(EXIT_FAILURE);
    }
    discovery = readTextFile(argv[1], &numUsuaris);
    if (discovery == NULL){
        return -2;
    }
    uint16_t poole_port = discovery[0].portPoole;
    uint16_t bowman_port = discovery[0].portBowman;

    if (poole_port < 1 || bowman_port < 1) {
        write(STDOUT_FILENO, "Error: Invalid port number(s)\n", sizeof("Error: Invalid port number(s)\n"));   
        exit(EXIT_FAILURE);
    }

     sockfd_poole = socket(AF_INET, SOCK_STREAM, 0);// Crear els sockets para Poole i Bowman
     sockfd_bowman = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd_poole < 0 || sockfd_bowman < 0) {
        perror("Error creating sockets");
        exit(EXIT_FAILURE);
    }

    // Configurar les estructures de direccio pels sockets
    struct sockaddr_in poole_addr, bowman_addr;
    memset(&poole_addr, 0, sizeof(poole_addr));
    poole_addr.sin_family = AF_INET;
    poole_addr.sin_addr.s_addr = INADDR_ANY;
    poole_addr.sin_port = htons(poole_port);

    memset(&bowman_addr, 0, sizeof(bowman_addr));
    bowman_addr.sin_family = AF_INET;
    bowman_addr.sin_addr.s_addr = INADDR_ANY;
    bowman_addr.sin_port = htons(bowman_port);

    if (bind(sockfd_poole, (struct sockaddr *)&poole_addr, sizeof(poole_addr)) < 0 ||
        bind(sockfd_bowman, (struct sockaddr *)&bowman_addr, sizeof(bowman_addr)) < 0) { // enllaçar socketss a direccions
        perror("Error binding socket");
        exit(EXIT_FAILURE);
    }

    if (listen(sockfd_poole, 5) < 0 || listen(sockfd_bowman, 5) < 0) {    // Escoltar en els dos sockets i deixem com a maxim 5 
        perror("Error listening on socket");
        exit(EXIT_FAILURE);
    }

    fd_set master_set;
    FD_ZERO(&master_set);
    FD_SET(sockfd_poole, &master_set);
    FD_SET(sockfd_bowman, &master_set);

    int max_sd = sockfd_poole > sockfd_bowman ? sockfd_poole : sockfd_bowman;
    init_poole_list(&pooleList);

    while (1) {
        fd_set read_set = master_set;
        if (select(max_sd + 1, &read_set, NULL, NULL, NULL) < 0) {        // Esperar activitats en algun socket
            perror("select");
            exit(EXIT_FAILURE);
        }
       
        if (FD_ISSET(sockfd_poole, &read_set)) {        // Verificar si nova conexio amb Poole
            waitSocketPoole(sockfd_poole,&pooleList);   
        }

        if (FD_ISSET(sockfd_bowman, &read_set)) {        // Verificar si nova conexio amb Bowman
            waitSocketBowman(sockfd_bowman,&pooleList);    ///         
        }      
    }
    freeAndClose();
    return 0;
}
