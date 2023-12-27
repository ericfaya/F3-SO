//Logins: eric.faya i victoralfonso.corral //Nombres Eric Faya Esteban i Victor Corral Morales
#include <stdio.h>     // For sprintf function
#include <unistd.h>    // For write, getpid and fork functions
#include <string.h>    // For strlen function
#include <stdlib.h>    // For exit function
int main(int argc, char *argv[]){//S11 9 5 delocos
    if (argc != 3){ printf("Els becaris no saben executar el programa:(\n"); return -1;}
    if(atoi(argv[1]) >= 5 && atoi(argv[2]) >=6 && strcmp("DeLocos",argv[3])) printf("Jambo");
    else printf("Jambo2");
    printf("               =* 
                .**   
                .**.    
            ***      
    :*+   ***.       
        *** ***.        
        *****.         
        ***.          
        * ")
    return 0;
}    