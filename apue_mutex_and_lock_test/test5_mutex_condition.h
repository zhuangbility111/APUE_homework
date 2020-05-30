#include <pthread.h>

// 主要是使用条件变量与互斥量来实现简单的消息队列
// 使用条件变量来判断消息队列的情况，也就是借助条件变量来实现阻塞和唤醒的功能
// 如果消息队列为空，则条件变量设为false，线程进入阻塞；不为空则为true，此时会唤醒线程继续往下执行
// 使用wait来等待条件变量，等待过程中阻塞，并释放对象锁
// 当有线程执行signal()时，说明条件变量为true，通知线程wait返回，获取对象锁，继续往下执行


// 消息结构体
struct msg {
    struct msg *m_next;
};

// 消息队列
struct msg *workq;

// 条件变量
pthread_cond_t qready = PTHREAD_COND_INITIALIZER;
// 互斥量
pthread_mutex_t qlock = PTHREAD_MUTEX_INITIALIZER;

// 处理消息
void process_msg() {
    // 消息队列
    struct msg *mp;

    // 循环处理消息
    for(;;) {
        // 先获取锁
        pthread_mutex_lock(&qlock);
        // 必须把判断布尔条件和wait()放到while循环中，而不能用if语句，原因是虚假唤醒
        // 此时线程被唤醒了，但是条件并不满足，这个时候如果不对条件进行检查而往下执行，就可能会导致后续的处理出现错误。 
        while (workq == NULL) {
            // 等待条件变量，在等待过程中释放锁
            // 等到条件变量为true时，说明消息队列已经准备好了，获取锁，并继续往下执行
            pthread_cond_wait(&qready, &qlock);
            // 从消息队列中获取消息，然后再释放锁
            mp = workq;
            workq = mp->m_next;
            pthread_mutex_unlock(&qlock);
            // 处理获取到的消息
        }
    }
}

// 添加消息到消息队列中
void enqueue_msg(struct msg *mp) {
    // 首先获取锁，才能操作消息队列
    pthread_mutex_lock(&qlock);
    mp->m_next = workq;
    workq = mp;
    // 释放锁
    pthread_mutex_unlock(&qlock);
    // 修改条件变量，通过条件变量唤醒相应的线程
    pthread_cond_signal(&qready);
}