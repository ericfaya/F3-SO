/*
* Solucio S8 Sistemes Operatius - Semàfors I - Biblio
* Curs 2023-24
*
* @author: Victor Xirau (⌐■_■)
*
*/
#define _GNU_SOURCE
#include <sys/ipc.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include "semaphore_v2.h"

#define print(x) write(1, x, strlen(x));

#define BIBLIO "BIENVENIDO A LA BIBLIO\n"
#define BIBLIO_DESCRIPTION "Gestionando aforos...\n"
#define CERRANDO "\n\nCerrando la biblioteca...\n"

#define MAX_STUDENTS 7          // Número máximo de estudiantes que pueden estar en la biblioteca al mismo tiempo
#define MAX_COMPUTERS 2         // Número máximo de ordenadores disponibles en la biblioteca
#define MAX_SEATS 4             // Número máximo de asientos disponibles en la biblioteca


semaphore sAlumnes;
semaphore sSeients;
semaphore sComputers;

void sortir(){
    write(1, CERRANDO, strlen(CERRANDO));
    SEM_destructor(&sAlumnes);
    SEM_destructor(&sSeients);
    SEM_destructor(&sComputers);
    signal(SIGINT, SIG_DFL);
    raise(SIGINT);
}


int main(){
    
    signal(SIGINT, sortir);

    SEM_constructor_with_name(&sAlumnes, ftok("biblio.c", 1));
    SEM_constructor_with_name(&sSeients, ftok("biblio.c", 2));
    SEM_constructor_with_name(&sComputers, ftok("biblio.c", 3));

    SEM_init(&sAlumnes, MAX_STUDENTS);
    SEM_init(&sSeients, MAX_SEATS);
    SEM_init(&sComputers, MAX_COMPUTERS);

    write(1, BIBLIO, strlen(BIBLIO));
    write(1, BIBLIO_DESCRIPTION, strlen(BIBLIO_DESCRIPTION));

    while(1){
        pause();
    }


}