#include<stdlib.h>
#include<stdio.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/ipc.h>
#include<sys/shm.h>
#include"/public/home/chenzhuang/unix_api_test/apue_hw_10.6/tell_wait.h"

/*
    使用 XSI 共享存储函数代替共享存储映射区，改写下图程序。
*/

#define NLOOPS  1000
#define SIZE    sizeof(long)

static int update(long* ptr) {
    return((*ptr)++);
}

inline void err_sys(char* error_msg) {
    perror(error_msg);
    exit(1);
}

int main(int argc, char** argv) {
    key_t shared_memeory_key;
    int counter;
    int shared_memory_id;
    void* area;
    pid_t pid;

    // 生成共享内存对应的key
    if ((shared_memeory_key = ftok(".", 'a')) < 0)
        err_sys("ftok error!\n");
    // 通过key生成标识符
    if ((shared_memory_id = shmget(shared_memeory_key, SIZE, IPC_CREAT|0666)) < 0)
        err_sys("shmget error!\n");
    // 获取指向共享内存的指针
    if ((area = shmat(shared_memory_id, 0, 0)) == (void*) -1)
        err_sys("shmat error!\n");
    
    tell_wait();

    if ((pid = fork()) < 0) {
        err_sys("fork error!\n");
    }
    // 父进程
    else if (pid > 0) {
        for (int i = 0; i < NLOOPS; i += 2) {
            // 对共享内存上的变量进行更新
            if ((counter = update((long*) area)) != i)
                printf("parent: expected %d, got %d", i, counter);

            tell_child(pid);
            wait_child();
        }
        printf("parent counter：%d\n", counter);
    }
    // 子进程
    else {
        for (int i = 1; i < NLOOPS + 1; i += 2) {
            wait_parent();
            if ((counter = update((long*) area)) != i)
                printf("child: expected %d, got %d", i, counter);

            tell_parent(getppid()); 
        }
        printf("child counter：%d\n", counter);
    }
    return 0;
}