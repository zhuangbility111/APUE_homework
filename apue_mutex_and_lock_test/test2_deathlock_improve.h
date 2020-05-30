#include <stdlib.h>
#include <pthread.h>

// test2的改进版，对哈希表的访问和对结构体的引用计数访问使用同一个互斥量
// 而原来结构体的锁则用来访问结构体的其他成员
// 简化修改哈希表中结构体的引用计数时繁琐的操作

#define NHASH 29
#define HASH(id) (((unsigned long)id)%NHASH)

struct foo* fh[NHASH];          // hash表

pthread_mutex_t hashlock = PTHREAD_MUTEX_INITIALIZER;       // 访问hash表时需要获取这个锁


struct foo {
    int             f_count;        // 引用计数
    pthread_mutex_t f_lock;         // 互斥量，锁住结构体内部成员的访问（除了引用计数的所有成员）
    int             f_id;           // 结构体id
    struct foo      *f_next;        // 用于hash表中，hash表解决冲突的方式是拉链法
};

// 初始化节点函数
struct foo* foo_alloc(int id) {
    struct foo* fp;
    int         idx;

    if((fp = (struct foo*)malloc(sizeof(struct foo))) != NULL) {
        // 申请内存成功，初始化结构体
        fp->f_count = 1;
        fp->f_id = id;
        if (pthread_mutex_init(&fp->f_lock, NULL) != 0) {
            // 初始化互斥量失败，回收内存
            free(fp);
            return NULL;
        }
        idx = HASH(id);
        // 获取hash表的锁，将结构体插入hash表中
        pthread_mutex_lock(&hashlock);
        fp->f_next = fh[idx];
        fh[idx] = fp;
        // 获取结构体的锁，进一步初始化
        pthread_mutex_lock(&fp->f_lock);
        // 为了保持获取锁的顺序，所以在获取到结构体的锁之后才能释放哈希表的锁
        pthread_mutex_unlock(&hashlock);
        // 对结构体做进一步的初始化
        pthread_mutex_unlock(&fp->f_lock);
    }
    return fp;
}

// 结构体的引用计数加1
void foo_hold(struct foo* fp) {
    // 获取哈希表的锁
    pthread_mutex_lock(&hashlock);
    // 引用计数加1
    fp->f_count++;
    pthread_mutex_unlock(&hashlock);
}


// 在哈希表中寻找指定id的结构体
struct foo* foo_find(int id) {
    struct foo  *fp;

    // 首先获取哈希表的锁，才能访问哈希表
    pthread_mutex_lock(&hashlock);
    // 遍历寻找
    for (fp = fh[HASH(id)]; fp != NULL; fp = fp->f_next) {
        if (fp->f_id == id) {
            // 直接更新引用计数，因为只需要有哈希表的锁就可以更新结构体的引用计数
            fp->f_count++;
            break;
        }
    }
    // 更新后，释放锁
    pthread_mutex_unlock(&hashlock);
    return fp;
}

// 引用计数减1，若结构体的引用计数为0，释放结构体
void foo_rele(struct foo* fp) {
    struct foo*     tfp;
    int             idx;

    // 获取哈希表锁，访问其中结构体的引用计数
    pthread_mutex_lock(&hashlock);
    // 不需要进一步获取结构体的锁，因为只需要哈希表的锁就可以访问结构体的引用计数
    // 简化了之前的操作
    // 之前需要先获取结构体锁，访问引用计数。如果需要删除哈希表中的结构体，还需要重新获得哈希表和结构体的锁
    if (--fp->f_count == 0) {
        // 在哈希表中删去这个结构体
        idx = HASH(fp->f_id);
        tfp= &fp[idx];
        if (tfp == fp) {
            fh[idx] = fp->f_next;
        }
        else {
            while (tfp->f_next != fp)
                tfp = tfp->f_next;
            tfp->f_next = fp->f_next;
        }
        // 释放锁
        pthread_mutex_unlock(&hashlock);
        pthread_mutex_destroy(&fp->f_lock);
        free(fp);          
    }
    else {
        pthread_mutex_unlock(&fp->f_lock);
    }
    
}