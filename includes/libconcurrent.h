#ifndef _LIBCONCURRENT_H
#define _LIBCONCURRENT_H

#include <pthread.h>

int conc_launch(void*(*routine)(void*), void* arg);
void conc_exit();
int mutex_init(pthread_mutex_t *lock);
int mutex_lock(pthread_mutex_t *lock);
int mutex_unlock(pthread_mutex_t *lock);
int mutex_destroy(pthread_mutex_t *lock);

#endif /*_LIBCONCURRENT_H*/
