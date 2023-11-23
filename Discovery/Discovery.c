#include "Discovery.h"
#include "config.h"
#include "PooleList.h"

void enviarAcknowledge(int newsock,int errorSocketOrNot,int bowmanOrPoole,PooleList *pooleList) {
    char *header;
    Frame *acknowledge_frame;
    if(errorSocketOrNot==-1 ){
        header = "[CON_KO]";
    }
    else {
        header = "CON_OK";
    }
  
    acknowledge_frame = (Frame *)malloc(sizeof(Frame));//cada cop que entra aqui,amplia una posicio el malloc el realloc
    char data2[FRAME_SIZE - 3 - strlen(header)]; // -3 por 'type' y 'header_length'.
    if(bowmanOrPoole == 0){ //Es 0(bowman) i cal fer una trama distinta
        PooleNode* pooleListMneysBalancejador=searchPooleListLessBowmans(pooleList);//balancejadorDeCarrega
        snprintf(data2, sizeof(data2), "%s&%s&%u", pooleListMneysBalancejador->info.userName,pooleListMneysBalancejador->info.ip,pooleListMneysBalancejador->info.port);
        pooleList->head->info.contador_bowmans++;
    }
    else{ //Es 1 (poole)
        snprintf(data2, sizeof(data2), " ");
    }

    errorSocketOrNot=build_frame(acknowledge_frame, 0x01, header, data2);
    char frame_buffer[FRAME_SIZE] = {0};
    pad_frame(acknowledge_frame, frame_buffer);
    
    write(newsock, frame_buffer, 256);
    free(acknowledge_frame->header);
    free(acknowledge_frame->data);
    free(acknowledge_frame);
}

void process_frame(Frame *frame, PooleList *list) {
    if (frame->type == 0x01 && strcmp(frame->header, "NEW_POOLE") == 0) {
        char userName[50], ip[INET_ADDRSTRLEN];
        int port;
        if (sscanf(frame->data, "%49[^&]&%15[^&]&%d", userName, ip, &port) == 3) {
            PooleInfo pooleInfo;
            strcpy(pooleInfo.userName, userName);
            strcpy(pooleInfo.ip, ip);
            pooleInfo.port = port;
            pooleInfo.contador_bowmans= 0;
            add_poole(list, pooleInfo);
        }
    }
}

void freeAndClose(PooleList *pooleList,int sockfd_poole,int sockfd_bowman){
    free_poole_list(pooleList);
    close(sockfd_poole);
    close(sockfd_bowman);
}

void waitSocketPoole(int sockfd_poole,PooleList *pooleList){
    struct sockaddr_in c_addr;
    socklen_t c_len = sizeof(c_addr);
    int newsock = accept(sockfd_poole, (struct sockaddr *)&c_addr, &c_len);
    if (newsock < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }
    
    Frame incoming_poole_frame; // Asegúrate de que la estructura Frame esté definida
    int errorSocketOrNot=receive_frame(newsock, &incoming_poole_frame);
    process_frame(&incoming_poole_frame, pooleList);
    enviarAcknowledge(newsock,errorSocketOrNot,1,pooleList);
}

void waitSocketBowman(int sockfd_bowman,PooleList *pooleList){
    struct sockaddr_in c_addr;
    socklen_t c_len = sizeof(c_addr);
    int newsock = accept(sockfd_bowman, (struct sockaddr *)&c_addr, &c_len);
    if (newsock < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }
    Frame incoming_poole_frame; // Asegúrate de que la estructura Frame esté definida
    int errorSocketOrNot=receive_frame(newsock, &incoming_poole_frame);
    if (strcmp(incoming_poole_frame.header, "EXIT") == 0){    
        removeBowmanFromPoole(pooleList,incoming_poole_frame.data); 
        return;       
    }
    enviarAcknowledge(newsock,errorSocketOrNot,0,pooleList);
}


int main(int argc, char *argv[]) {
    Discovery *discovery;
    int numUsuaris;

    if (argc < 2) {
        char *buffer;
        asprintf(&buffer,"Usage: %s <Poole port> <Bowman port>\n", argv[0]);  
        write(STDOUT_FILENO, buffer, strlen(buffer));   
        free(buffer);
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

    int sockfd_poole = socket(AF_INET, SOCK_STREAM, 0);// Crear els sockets para Poole i Bowman
    int sockfd_bowman = socket(AF_INET, SOCK_STREAM, 0);
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
    PooleList pooleList;
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
            waitSocketBowman(sockfd_bowman,&pooleList);            
        }      
    }
    freeAndClose(&pooleList,sockfd_bowman,sockfd_poole);
    return 0;
}
