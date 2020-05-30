#include <stdlib.h>
#include <pthread.h>

// 版本1，只有一个互斥量的版本

struct foo {
    int             count;        // foo的引用计数，为0时对foo进行释放
    pthread_mutex_t lock;         // 互斥量
    int             id;
};


// 根据id动态创建一个foo对象
struct foo* foo_alloc(int id) {
    struct foo* fp;

    // 对foo进行初始化
    if ((fp = (struct foo*)malloc(sizeof(struct foo))) != NULL) {
        fp->count = 1;
        fp->id = id;
        // 接着对互斥量进行初始化
        if (pthread_mutex_init(&fp->lock, NULL) != 0) {
            // 初始化失败，释放内存，返回
            free(fp);
            return NULL;
        }

    }

    return fp;
}

// 添加一个foo对象的引用，引用计数加1
void foo_hold(struct foo* fp) {
    // 先锁住互斥量，获取锁
    pthread_mutex_lock(&fp->lock);
    fp->count++;
    // 释放锁
    pthread_mutex_unlock(&fp->lock);
}

// 删除一个foo对象的引用，引用计数减1
// 引用计数为0时释放foo对象
void foo_rele(struct foo* fp) {
    // 先获取锁
    pthread_mutex_lock(&fp->lock);
    fp->count--;
    // 如果引用计数为0，释放对象
    if (fp->count == 0) {
        // 先释放锁，并销毁锁
        pthread_mutex_unlock(&fp->lock);
        pthread_mutex_destroy(&fp->lock);
        // 释放内存对象
        free(fp);
    }
    // 如果不为0，释放锁就行了
    else {
        pthread_mutex_unlock(&fp->lock);
    }
}