#ifndef MOD_SEMAPHORE_H
#define MOD_SEMAPHORE_H

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

// Struct and union definitions...

typedef struct {
    int shmid;
} semaphore;

// Function declarations...

int SEM_constructor_with_name(semaphore * sem, key_t key);
int SEM_constructor(semaphore * sem);
int SEM_init(const semaphore * sem, const int v);
int SEM_destructor(const semaphore * sem);
int SEM_wait(const semaphore * sem);
int SEM_signal(const semaphore * sem);

#endif /* MOD_SEMAPHORE_H */
