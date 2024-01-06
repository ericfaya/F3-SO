/*
* Solucio S10 Sistemes Operatius - Cues de Missatges Cocina
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
#include <pthread.h>

#define printF(x) write(1, x, strlen(x));

#define ERR_ARG "Porfavor introduce: ./cocina <num_cocineros>\n"
#define ERR_MSG "Error al enviar el mensaje\n"
#define ERR_COLA "Error al crear la cola de mensajes\n"
#define COCINERO_RECIBIDO "Cocinero %d Recibido!\n\tNumero Ingredientes: %d\n\tTiempo x Ingrediente: %d\n\n"
#define YA_ESTAN_TODOS "Ya están todos los cocineros estan trabajando!\nEsperando a que finalizen....\n"
#define DESPEDIDA "Perfecto! Espero que hayan cocinado bien!\n"
#define EMPIEZA "He empezado a cocinar"
#define RECETA_COMPLETADA "La receta está %.2f%% completada"
#define ACABAR_COCINAR "He acabado de cocinar"
#define ERR_DELETING_QUEUE "Error al eliminar la cola de mensajes\n"

typedef struct {
    long mtype;
    char mtext[256];
    int idCocinero;
    int numIngredientes;
    int tiempoIngrediente;
}Message;

int mq_id;

void nothing(){
    signal(SIGINT, nothing);
}

void* threadFunc(void* arg){
    Message msg = *(Message*)arg;

    memset(msg.mtext, 0, 256*sizeof(char));
    
    snprintf(msg.mtext, 256, EMPIEZA);
    if (msgsnd(mq_id, &msg, 256*sizeof(char)+3*sizeof(int), 0) == -1) {
        perror("msgsnd");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < msg.numIngredientes; i++) {
        snprintf(msg.mtext, 256, RECETA_COMPLETADA, ((i+1)/(msg.numIngredientes*1.0))*100);
        if (msgsnd(mq_id, &msg, 256*sizeof(char)+3*sizeof(int), 0) == -1) {
            perror("msgsnd");
            exit(EXIT_FAILURE);
        }
        sleep(msg.tiempoIngrediente);
    }

    snprintf(msg.mtext, 256, ACABAR_COCINAR);
    if (msgsnd(mq_id, &msg, 256*sizeof(char)+3*sizeof(int), 0) == -1) {
        perror("msgsnd");
        exit(EXIT_FAILURE);
    }
    
    pthread_cancel(pthread_self());
    pthread_detach(pthread_self());
    return NULL;
}


int main(int argc, char*argv[]) {
    if(argc != 2){
        printF(ERR_ARG);
    }else{
        char*buff;

        signal(SIGINT, nothing);

        key_t key = ftok("S10_cocinero.c", 1);
        mq_id = msgget(key, IPC_CREAT | 0666);
        if (mq_id == -1) {
            printF(ERR_COLA);
            exit(EXIT_FAILURE);
        }
         Message msg;

        int numCocineros = atoi(argv[1]);
        pthread_t* threads = malloc(sizeof(pthread_t)*numCocineros);

        int numRecibidos=0;
        while(1){
            ssize_t bytes_read = msgrcv(mq_id, &msg, 256*sizeof(char)+3*sizeof(int), -numCocineros, 0);
            if (bytes_read < 0 ) {
                printF(ERR_MSG);
            }else{

                asprintf(&buff, COCINERO_RECIBIDO, msg.idCocinero, msg.numIngredientes, msg.tiempoIngrediente);
                printF(buff);
                free(buff);

                Message m2;
                m2.idCocinero = msg.idCocinero;
                m2.mtype = 1000 + msg.idCocinero;
                m2.numIngredientes = msg.numIngredientes;
                m2.tiempoIngrediente = msg.tiempoIngrediente;
                pthread_create(&threads[msg.idCocinero-1], NULL, threadFunc, &m2);
                numRecibidos++;
                if(numRecibidos == numCocineros){
                    break;
                }
            }
        }

        printF(YA_ESTAN_TODOS);

        for(int i=0; i<numCocineros; i++){
            pthread_join(threads[i], NULL);
        }

        printF(DESPEDIDA);

        free(threads);
       
        if (msgctl(mq_id, IPC_RMID, NULL) == -1) {
            printF(ERR_DELETING_QUEUE);
            exit(EXIT_FAILURE);
        }
    }

    return 0;
}
