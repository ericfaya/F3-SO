/*
* Solucio S10 Sistemes Operatius - Cues de Missatges Cocinero
* Curs 2023-24
*
* @author: Victor Xirau (⌐■_■)
*
*/
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <signal.h>

#define printF(x) write(1, x, strlen(x));

#define ERR_ARG "Porfavor introduce: ./cocinero <id_cocinero> <num_ingredientes> <tiempo_ingrediente>\n"
#define ERR_MSG "Error al enviar el mensaje\n"
#define ERR_COLA "Error al crear la cola de mensajes\n"
#define ENVIAR_RECETA "Enviando receta a la cocina...\n"
#define EMPEZAR_COCINAR "Empezando a cocinar\n"
#define ACABAR_COCINAR "He acabado de cocinar"

typedef struct {
    long mtype;
    char mtext[256];
    int idCocinero;
    int numIngredientes;
    int tiempoIngrediente;
}Message;

void nothing(){
    signal(SIGINT, nothing);
}

int main(int argc, char*argv[]) {

    if(argc != 4){
        printF(ERR_ARG);
    }else{
        key_t key = ftok("S10_cocinero.c", 1);
        int mq_id = msgget(key, IPC_CREAT | 0666);
        if (mq_id == -1) {
            printF(ERR_COLA);
            exit(EXIT_FAILURE);
        }

        signal(SIGINT, nothing);

        // Enviamos la información inicial a la cocina con nuestra info
        Message msg;
        msg.idCocinero = atoi(argv[1]);
        msg.numIngredientes = atoi(argv[2]);
        msg.tiempoIngrediente = atoi(argv[3]);
        msg.mtype = msg.idCocinero;
        memset(msg.mtext, 0, 256*sizeof(char));

        
        printF(ENVIAR_RECETA);
        if (msgsnd(mq_id, &msg, 256*sizeof(char)+3*sizeof(int), 0) == -1) {
            printF(ERR_MSG);
            exit(EXIT_FAILURE);
        }

        printF(EMPEZAR_COCINAR);
    
        while (1) {
                if (msgsnd(mq_id, &msg, 256*sizeof(char)+3*sizeof(int), 0) == -1) {

            ssize_t bytes_read = msgrcv(mq_id, &msg, 256*sizeof(char)+3*sizeof(int), 1000 + msg.idCocinero, 0);
            if (bytes_read == -1) {
                printF(ERR_COLA);
                exit(EXIT_FAILURE);
            }

            printf("%s\n", msg.mtext);

            if (strcmp(msg.mtext, ACABAR_COCINAR) == 0) {
                break;
            }
        }
    }

    return 0;
}