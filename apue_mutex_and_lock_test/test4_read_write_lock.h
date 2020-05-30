#include <stdlib.h>
#include <pthread.h>

// 使用读写锁来维护一个作业请求队列，每一个作业由一个线程负责执行
// 实际上就是一个用链表实现的作业队列
// 读时申请读锁，写时申请写锁
// 有多个读锁，但是只有一个写锁

// 作业结构体
struct job {
    struct job      * j_next;
    struct job      * j_prev;
    pthread_t         j_id;         // 作业所属线程的id
};

// 作业队列
struct queue {
    struct job      * q_head;
    struct job      * q_tail;
    pthread_rwlock_t  q_lock;       // 队列的读写锁
};

int queue_init(struct queue *qp) {
    int err;

    qp->q_head = NULL;
    qp->q_tail = NULL;
    // 初始化队列的读写锁
    err = pthread_rwlock_init(&qp->q_lock, NULL);

    if (err != 0)
        return err;
    return 0;
}

// 插入一个作业到队列的头部
void job_insert(struct queue *qp, struct job* jp) {
    // 首先获取写锁
    pthread_rwlock_wrlock(&qp->q_lock);
    jp->j_next = qp->q_head;
    jp->j_prev = NULL;
    if (qp->q_head != NULL) {
        qp->q_head->j_prev = jp;
    }
    else {
        qp->q_tail = jp;
    }
    qp->q_head = jp;
    // 释放锁
    pthread_rwlock_unlock(&qp->q_lock);
}

// 插入一个作业到队列的尾部
void job_append(struct queue* qp, struct job* jp) {
    // 首先获取写锁
    pthread_rwlock_wrlock(&qp->q_lock);
    if (qp->q_tail == NULL) {
        qp->q_head = jp;
        jp->j_prev = NULL;
    }
    else {
        qp->q_tail->j_next = jp;
        jp->j_prev = qp->q_tail;
    }
    jp->j_next = NULL;
    qp->q_tail = jp;
    // 释放写锁
    pthread_rwlock_unlock(&qp->q_lock);
}

// 从作业队列中删除一个指定的作业
void job_remove(struct queue* qp, struct job *jp) {
    // 获取写锁
    pthread_rwlock_wrlock(&qp->q_lock);
    // 如果是头节点
    if (jp == qp->q_head) {
        qp->q_head = jp->j_next;
        if (qp->q_head != NULL) {
            qp->q_head->j_prev = NULL;
        }
        else {
            qp->q_tail = NULL;
        }
    }
    // 如果是尾节点
    else if (jp == qp->q_tail) {
        qp->q_tail = jp->j_prev;
        if (qp->q_tail != NULL) {
            qp->q_tail->j_next = NULL;
        }
        else {
            qp->q_head = NULL;
        }
    }
    // 如果是内部节点
    else {
        jp->j_prev->j_next = jp->j_next;
        jp->j_next->j_prev = jp->j_prev;
    }
    free(jp);
    // 释放写锁
    pthread_rwlock_unlock(&qp->q_lock);
}

// 根据线程id在队列中寻找一个作业
struct job* job_find(struct queue* qp, pthread_t id) {
    // 因为只涉及到读，所以只需要读锁
    if(pthread_rwlock_rdlock(&qp->q_lock) != 0)
        return NULL;
    struct job* jp;
    for (jp = qp->q_head; jp != NULL; jp = jp->j_next) {
        if (pthread_equal(jp->j_id, id))
            break;
    }
    // 释放读锁
    pthread_rwlock_unlock(&qp->q_lock);
    return jp;
}