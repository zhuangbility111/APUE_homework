#include <pthread.h>
#include <limits.h>
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>

#define NUM_THREADS 8           // 线程数量
#define NUM_TOTAL   8000000L    // 待排序元素的数量
#define NUM_PER_THREAD  (NUM_TOTAL/NUM_THREADS)    // 每个线程负责排序的元素数量


long nums[NUM_TOTAL];
long snums[NUM_TOTAL];      // 保存排序的结果

pthread_barrier_t barrier;  // 屏障

#ifdef SOLARIS
#define heapsort qsort
#else
extern int heapsort(void *, size_t, size_t, int (*)(const void*, const void*));
#endif

// 比较大小函数（仿函数）
int compare_long(const void* arg1, const void* arg2) {
    long l1 = *(long *)arg1;
    long l2 = *(long *)arg2;

    if (l1 == l2)
        return 0;
    else if (l1 < l2)
        return -1;
    else    
        return 1;
}

// 工作线程调用的函数，用于对它负责的那一部分数据进行排序
void* thread_fn(void* args) {
    long    idx = (long)args;
    // 调用堆排序
    heapsort(&nums[idx], NUM_PER_THREAD, sizeof(long), compare_long);
    // 接着屏障阻塞，同步其他工作线程
    pthread_barrier_wait(&barrier);
    return (void*)0;
}

// 对所有工作线程的排序结果进行合并
void merge() {
    long    idx[NUM_THREADS];           // 保存每一个线程内部当前准备进行合并的元素的索引
    long    i, min_idx, s_idx, num;     

    // 先设置每一个线程的起始元素的索引
    for (i = 0; i < NUM_THREADS; i++) {
        idx[i] = i * NUM_PER_THREAD;
    }
    for (s_idx = 0; s_idx < NUM_TOTAL; s_idx++) {
        num = LONG_MAX;
        // 从所有工作线程中挑选出最小的那个元素，放入结果中
        for (i = 0; i < NUM_THREADS; i++) {
            if (idx[i] < (i+1)*NUM_PER_THREAD && nums[idx[i]] < num) {
                num = nums[idx[i]];
                min_idx = i;
            }
        }
        snums[s_idx] = nums[idx[min_idx]];
        idx[min_idx]++;
    }
}

int main() {
    unsigned long       i;
    struct timeval      start, end;
    long long           start_usec, end_usec;
    double              elapsed;
    int                 err;
    pthread_t           tid;

    // 随机数
    srandom(1);
    for (i = 0; i < NUM_TOTAL; i++) {
        nums[i] = random();
    }

    // 获取当前的时间，记录开始时间
    gettimeofday(&start, NULL);

    // 初始化barrier
    pthread_barrier_init(&barrier, NULL, NUM_THREADS+1);

    // 创建工作线程，执行排序
    for(i = 0; i < NUM_THREADS; i++) {
        // 创建线程，并且指定线程中执行的函数，以及传入的函数参数
        err = pthread_create(&tid, NULL, thread_fn, (void*)(i*NUM_PER_THREAD));
        if (err != 0)
            printf("can't create thread\n");
    }

    // 接着主线程等待，同步所有工作线程
    pthread_barrier_wait(&barrier);

    // 合并每个线程的排序结果
    merge();

    // 记录结束时间
    gettimeofday(&end, NULL);

    start_usec = start.tv_sec * 1000000 + start.tv_usec;
    end_usec = end.tv_sec * 1000000 + end.tv_usec;
    elapsed = (double) (end_usec - start_usec) / 1000000.0;
    printf("sort took %.4f seconds\n", elapsed);
    for (i = 0; i < NUM_TOTAL; i++) {
        printf("%ld\n", snums[i]);
    }
    return 0;
}