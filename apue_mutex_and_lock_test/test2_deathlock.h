#include <stdlib.h>
#include <pthread.h>

// test1的升级版，多了一个hash表
// 访问结构体内的成员有一个互斥量f_lock，访问哈希表的成员有一个互斥量hashlock
// 为了防止死锁，固定一个获取锁的顺序。在需要更新哈希表中某个结构体的信息时，先获取哈希表的锁，再获取结构体的锁

#define NHASH 29
#define HASH(id) (((unsigned long)id)%NHASH)

struct foo* fh[NHASH];          // hash表

pthread_mutex_t hashlock = PTHREAD_MUTEX_INITIALIZER;       // 访问hash表时需要获取这个锁

struct foo {
    int             f_count;        // 引用计数
    pthread_mutex_t f_lock;         // 互斥量，锁住结构体内部成员的访问
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
    // 获取结构体的锁
    pthread_mutex_lock(&fp->f_lock);
    // 引用计数加1
    fp->f_count++;
    pthread_mutex_unlock(&fp->f_lock);
}

// 在哈希表中寻找指定id的结构体
struct foo* foo_find(int id) {
    struct foo  *fp;

    // 首先获取哈希表的锁，才能访问哈希表
    pthread_mutex_lock(&hashlock);
    // 遍历寻找
    for (fp = fh[HASH(id)]; fp != NULL; fp = fp->f_next) {
        if (fp->f_id == id) {
            // 在函数中获取锁，增加引用计数
            foo_hold(fp);
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

    // 获取结构体的锁，访问它的引用计数
    pthread_mutex_lock(&fp->f_lock);
    if (fp->f_count == 1) {
        // 因为释放需要操作哈希表，所以为了防止发生死锁，得按顺序来获得锁
        // 所以必须先释放结构体的锁，然后获得哈希表的锁，最后再重新获取结构体的锁
        pthread_mutex_unlock(&fp->f_lock);
        pthread_mutex_lock(&hashlock);
        pthread_mutex_lock(&fp->f_lock);
        // 获取锁的过程中，可能有其他线程访问了结构体，该结构体的引用计数会改变，所以需要重新检查一遍
        // 如果不为1，则只需要减1就可以了
        if (fp->f_count != 1) {
            fp->f_count--;
            pthread_mutex_unlock(&fp->f_lock);
            pthread_mutex_unlock(&hashlock);
            return;
        }
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
        pthread_mutex_unlock(&fp->f_lock);
        pthread_mutex_destroy(&fp->f_lock);
        free(fp);
            
    }
    else {
        fp->f_count--;
        pthread_mutex_unlock(&fp->f_lock);
    }
}
