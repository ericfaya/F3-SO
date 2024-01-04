/*
* Solucio S8 Sistemes Operatius - Semàfors I - Student
* Curs 2023-24
*
* @author: Victor Xirau (⌐■_■)
*
*/
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <time.h>
#include "semaphore_v2.h"

#define print(x) write(1, x, strlen(x));

#define WAITING_TO_ENTER "Estoy esperando para entrar en la biblioteca...\n"
#define DENTRO_ESPERANDO_ASIENTO "Estoy dentro! Ahora estoy esperando a tener un asiento...\n"
#define TE_SEIENT "Porfin! Tengo un asiento. Hoy estudiaré %d s. Empiezo a estudiar...\n"
#define STUDY_END "Ya he terminado! Me voy. Hoy he pasado %d s en la bilbio, suficiente :| \n"
#define VOL_ORDE "Quiero usar el ordenador! Libero mi sitio. Llevaba %d s de estudio de %d s totales.\n"
#define ESPERA_ORDE "Estoy esperando a poder usarlo :D \n"
#define LO_TENGO "Tengo el ordenador! Lo quiero usar durante %d s\n"
#define DONE_ORDE  "He acabado de usar el ordenador. Me voy a estudiar de nuevo.\n"
#define BACK_TO_STUDY "Estoy esperando a tener sitio...\n"
#define SIGO_ESTUDIANDO "Lo tengo! Voy a seguir que solo me quedan %d s más de %d s totales. ;)\n"

#define MAX_STUDY_TIME 21           // Tiempo máximo de estudio para un estudiante en segundos
#define MIN_STUDY_TIME 3            // Tiempo mínimo de estudio para un estudiante en segundos
#define MAX_COMPUTER_TIME 2         // Tiempo máximo de uso de un ordenador en segundos
#define MIN_COMPUTER_TIME 10        // Tiempo mínimo de uso de un ordenador en segundos


semaphore sAlumnes;
semaphore sSeients;
semaphore sComputers;

int study_time;
int t=0;

int totalTime=0;


void nothing(int s){
    signal(s, nothing);
}

void contarTemps(){
    char* buffer;
    t++;
    // Si t es múltiplo de 3 es que han pasado 3s desde la última comprobación, y hay que mirar si quiere el ordenador
    if(t % 3 == 0 && t != study_time){
        srand(time(NULL));
        if(rand() % 2 == 1){
            // En caso que si, ignoraremos el último signal lanzado de alarma
            signal(SIGALRM, nothing);

            asprintf(&buffer, VOL_ORDE, t, study_time);
            print(buffer);
            free(buffer);

            // Liberamos nuestro asiento
            SEM_signal(&sSeients);

            print(ESPERA_ORDE);

            // Esperamos a que haya un ordenador libre
            SEM_wait(&sComputers);

            // Calculamos el tiempo que usará el ordenador
            srand(time(NULL));
            int computer_time = rand() % (MAX_COMPUTER_TIME - MIN_COMPUTER_TIME + 1) + MIN_COMPUTER_TIME;

            // Sumamos el tiempo que ha estado usando el ordenador al tiempo total
            totalTime += computer_time;
            
            asprintf(&buffer, LO_TENGO, computer_time);
            print(buffer);
            free(buffer);
            
            // Esperamos el tiempo que use el ordenador
            sleep(computer_time);

            print(DONE_ORDE);

            // Liberamos el ordenador
            SEM_signal(&sComputers);

            print(BACK_TO_STUDY);

            // Esperamos a que haya un asiento libre
            SEM_wait(&sSeients);

            asprintf(&buffer, SIGO_ESTUDIANDO, study_time - t, study_time);
            print(buffer);
            free(buffer);

            // Volvemos a lanzar la alarma
            alarm(1);
        }
    }
    signal(SIGALRM, contarTemps);
}


int main(){
    
    signal(SIGALRM, contarTemps);
    signal(SIGINT, nothing);

    SEM_constructor_with_name(&sAlumnes, ftok("biblio.c", 1));
    SEM_constructor_with_name(&sSeients, ftok("biblio.c", 2));
    SEM_constructor_with_name(&sComputers, ftok("biblio.c", 3));

    print(WAITING_TO_ENTER);

    // Esperamos a que haya menos de 7 alumnos en la biblioteca
    SEM_wait(&sAlumnes);

    print(DENTRO_ESPERANDO_ASIENTO);

    // Esperamos a que haya un asiento libre
    SEM_wait(&sSeients);
    
    // Calculamos el tiempo de estudio
    srand(time(NULL));
    study_time = rand() % (MAX_STUDY_TIME - MIN_STUDY_TIME + 1) + MIN_STUDY_TIME;
    t = 0;

    totalTime += study_time;

    char* buffer;
    asprintf(&buffer, TE_SEIENT, study_time);
    print(buffer);
    free(buffer);

    // Hacemos interrupcions de 1s, para poder contar el tiempo de estudio y ver si el alumno quiere usar el ordenador
    while(1){
        alarm(1);
        if(t<study_time){
            pause();
        }else{
            signal(SIGALRM, nothing);
            break;
        }
    }

    asprintf(&buffer, STUDY_END, totalTime);
    print(buffer);
    free(buffer);


    SEM_signal(&sSeients);
    SEM_signal(&sAlumnes);

}