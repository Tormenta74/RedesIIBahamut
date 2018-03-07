#include <pthread.h>

#include "globals.h"
#include "libconcurrent.h"

// conc = concurrent / concurrency

/*
 * Description: Creates a thread executing the attention routine with its args.
 *
 * In:
 * void*(*routine)(void*): a function to the pointer to be executed
 * (see includes/server.h:attention_routine definition)
 * void *arg: generic pointer to whatever argument the function needs
 *
 * Return: ERR in case of sanity failure. pthread_create's return otherwise.
 */
int conc_launch(void*(*routine)(void*), void *arg) {
    pthread_t tid;
    if (routine) {
        return pthread_create(&tid, NULL, routine, arg);
    }
    return ERR;
}

/*
 * Description: Calls the thread-exit function of pthread
 */
void conc_exit() {
    pthread_exit(NULL);
}

/*
 * Description: Wrapper around the mutex init function of pthread.
 *
 * In:
 * pthread_mutex_t *lock: the mutex to be initialized
 *
 * Return: ERR in case of sanity failure. pthread_mutex_init's return otherwise.
 */
int mutex_init(pthread_mutex_t *lock) {
    if (lock) {
        return pthread_mutex_init(lock, NULL);
    }
    return ERR;
}

/*
 * Description: Wrapper around the lock function of pthread
 *
 * In:
 * pthread_mutex_t *lock: the mutex to be locked
 *
 * Return: ERR in case of sanity failure. pthread_mutex_lock's return otherwise.
 */
int mutex_lock(pthread_mutex_t *lock) {
    if (lock) {
        return pthread_mutex_lock(lock);
    }
    return ERR;
}

/*
 * Description: Wrapper around the unlock function of pthread
 *
 * In:
 * pthread_mutex_t *lock: the mutex to be unlocked
 *
 * Return: ERR in case of sanity failure. pthread_mutex_unlock's return otherwise.
 */
int mutex_unlock(pthread_mutex_t *lock) {
    if (lock) {
        return pthread_mutex_unlock(lock);
    }
    return ERR;
}

/*
 * Description: Wrapper around the mutex destroy function of pthread
 *
 * In:
 * pthread_mutex_t *lock: the mutex to be unlocked
 *
 * Return: ERR in case of sanity failure. pthread_mutex_destroy's return otherwise.
 */
int mutex_destroy(pthread_mutex_t *lock) {
    if (lock) {
        return pthread_mutex_destroy(lock);
    }
    return ERR;
}

