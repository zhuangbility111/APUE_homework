#include "pthread_barrier.h"

#define NUM_OF_THREAD 4

void* thread_aux(void* arg) {
    my_pthread_barrier_t* barrier = (my_pthread_barrier_t*)arg;
    pthread_t tid = pthread_self();
    printf("Thread %lu barrier wait!\n", (unsigned long)tid);
    my_pthread_barrier_wait(barrier);
    printf("Thread %lu just go!\n", (unsigned long)tid);
    return NULL;
}

int main(int argc, char** argv) {
    my_pthread_barrier_t* barrier = (my_pthread_barrier_t*)malloc(sizeof(my_pthread_barrier_t));
    my_pthread_barrier_init(barrier, NULL, NUM_OF_THREAD);
    pthread_t tid;
    for (int i = 0; i < NUM_OF_THREAD; i++) {
        pthread_create(&tid, NULL, thread_aux, (void*)barrier);
        sleep(1);
    }
    return 0;
}