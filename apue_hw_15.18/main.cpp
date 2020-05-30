#include<stdlib.h>
#include<stdio.h>
#include<unistd.h>
#include<string.h>
#include<sys/types.h> 
#include <sys/wait.h>
#include<sys/ipc.h>
#include<sys/shm.h>
#include<sys/msg.h>
#include<sys/sem.h>
#include<sys/mman.h>
#include<fcntl.h>
#include<semaphore.h>

/*
使用 POSIX 信号量函数改写 15.15 题图中的程序，实现父子进程的交替。
*/


#define NLOOPS  1000000
#define SIZE    sizeof(long)

static int update(long* ptr) {
    return((*ptr)++);
}

inline void err_sys(char* error_msg) {
    perror(error_msg);
    exit(1);
}


int main(int argc, char** argv) {
    int fd, counter;
    pid_t pid;
    void* area;
    sem_t* semo_1;
    sem_t* semo_2;
    char* semo_1_name = "hello";
    char* semo_2_name = "hellohello";
    
    if ((semo_1 = sem_open(semo_1_name, O_CREAT, 0666, 0)) == SEM_FAILED) 
        err_sys("sem_open1 error!\n");
    if ((semo_2 = sem_open(semo_2_name, O_CREAT, 0666, 0)) == SEM_FAILED) 
        err_sys("sem_open2 error!\n");

    if ((fd = open("./lock_file", O_RDWR | O_CREAT)) < 0) 
        err_sys("open error!\n");
    // 清空文件内容
    ftruncate(fd,0);
    // 重新设置文件偏移量
    lseek(fd, 0, SEEK_SET);
    // 因为写入之后，读出来是按照字符的ASCII码值读出来的，因此如果写入的是'0'，那么读的是它的ASCII码值42
    // 因此必须写入'\0'，它的ASCII码值是0
    write(fd, "\0", 1);
    if ((area = mmap(0, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)) == MAP_FAILED)
        err_sys("mmap error!\n");

    if ((pid = fork()) < 0) {
        err_sys("fork error!\n");
    }
    // 父进程
    else if (pid > 0) {
        for (int i = 0; i < NLOOPS; i += 2) {
            // 获取semo_1信号量
            sem_wait(semo_1);
            if ((counter = update((long*) area)) != i)
                printf("parent: expected %d, got %d\n", i, counter);
            // 释放semo_2信号量
            sem_post(semo_2);
        }
        printf("parent counter：%d\n", counter);
        if (waitpid(pid, NULL, 0) < 0)
            err_sys("wait error!\n");
        sem_close(semo_1);
        sem_unlink(semo_1_name);
        sem_close(semo_2);
        sem_unlink(semo_2_name);
        close(fd);
    }
    // 子进程
    else {
        for (int i = 1; i < NLOOPS + 1; i += 2) {
            // 释放semo_1信号量
            sem_post(semo_1);
            // 获取semo_2信号量
            sem_wait(semo_2);
            if ((counter = update((long*) area)) != i)
                printf("child: expected %d, got %d\n", i, counter);
        }
        printf("child counter：%d\n", counter);
        close(fd);
    }

    return 0;
}