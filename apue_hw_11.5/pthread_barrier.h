#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

// 自定义的pthread_barrier_t结构体
typedef struct{
    pthread_mutex_t mutex;      // 互斥量，用于给条件变量加锁
    pthread_cond_t  cond;       // 条件变量，条件满足时对wait()阻塞的函数进行唤醒
    int             limit;      // 屏障等待的总线程数                 
    int             count;      // 当前在屏障处等待的线程数
    int             phase;      // 条件变量对应的条件状态，用于判断唤醒后条件状态是否改变
    // 因为屏障需要重复使用，所以条件状态不能简单判断为某个值，而是与阻塞前的状态进行对比
} my_pthread_barrier_t;

typedef struct {

} my_pthread_barrierattr_t;

// 初始化pthread_barrier_t函数
int my_pthread_barrier_init(my_pthread_barrier_t *barrier,
                            const my_pthread_barrierattr_t *attr,
                            unsigned int count);

// 反初始化pthread_barrier_t函数
int my_pthread_barrier_destroy(my_pthread_barrier_t *barrier);

// 核心函数，用于设置屏障，等待所有线程执行到此位置
int my_pthread_barrier_wait(my_pthread_barrier_t *barrier);