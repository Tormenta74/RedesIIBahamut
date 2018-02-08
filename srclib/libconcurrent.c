#include <pthread.h>

#include "globals.h"
#include "libconcurrent.h"

// conc = concurrent / concurrency

/* routine es una función con signature igual que attention_routine,
 * es decir, compatible con los hilos de pthread */
int conc_launch(void*(*routine)(void*), void* arg) {
    pthread_t tid;
    if(routine) return pthread_create(&tid, NULL, routine, arg);
    return ERR;
}

void conc_exit() {
    pthread_exit(NULL);
}

int mutex_init(pthread_mutex_t *lock) {
    if(lock) return pthread_mutex_init(lock,NULL);
    return ERR;
}

int mutex_lock(pthread_mutex_t *lock) {
    if(lock) return pthread_mutex_lock(lock);
    return ERR;
}

int mutex_unlock(pthread_mutex_t *lock) {
    if(lock) return pthread_mutex_unlock(lock);
    return ERR;
}

int mutex_destroy(pthread_mutex_t *lock) {
    if(lock) return pthread_mutex_destroy(lock);
    return ERR;
}

