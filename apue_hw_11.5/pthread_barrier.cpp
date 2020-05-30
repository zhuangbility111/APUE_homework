#include "pthread_barrier.h"

// 初始化barrier
int my_pthread_barrier_init(my_pthread_barrier_t *barrier, const my_pthread_barrierattr_t *attr, unsigned int count) {
    if (count == 0) 
        return -1;
    if (pthread_mutex_init(&barrier->mutex, NULL) < 0) {
        return -1;
    }
    if (pthread_cond_init(&barrier->cond, NULL) < 0) {
        pthread_mutex_destroy(&barrier->mutex);
        return -1;
    }
    barrier->phase = 0;
    barrier->limit = count;
    barrier->count = 0;
    return 0;
}

// 反初始化barrier
int my_pthread_barrier_destroy(my_pthread_barrier_t *barrier) {
    pthread_mutex_destroy(&barrier->mutex);
    pthread_cond_destroy(&barrier->cond);
}

// 核心函数，用于设置屏障，等待所有线程执行到此位置
int my_pthread_barrier_wait(my_pthread_barrier_t *barrier) {
    // 先获取互斥锁，增加已经到达此位置的线程数
    pthread_mutex_lock(&barrier->mutex);
    barrier->count++;
    // 如果count等于limit，说明所有的线程都已经到达屏障处，修改条件变量，并使用条件变量函数唤醒所有线程，并且释放锁
    if (barrier->count >= barrier->limit) {
        barrier->count = 0;
        barrier->phase++;
        pthread_cond_broadcast(&barrier->cond);
        pthread_mutex_unlock(&barrier->mutex);
        return 0;
    }
    // 如果不等于，说明需要在这里阻塞等待其他线程
    else {
        // 因为要将屏障重复使用，所以条件状态不能简单地判断为等于某个值，而是与阻塞前的条件状态进行队比
        unsigned phase = barrier->phase;
        // 循环判断是否满足条件状态，如果被唤醒后条件状态不满足，则继续进入阻塞
        do {
            pthread_cond_wait(&barrier->cond, &barrier->mutex);
        } while(phase == barrier->phase);
        // wait()返回后会重新持有互斥锁，因此需要释放锁
        pthread_mutex_unlock(&barrier->mutex);
        return 0;
    }
}