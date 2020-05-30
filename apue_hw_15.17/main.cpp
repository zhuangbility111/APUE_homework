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

/*
使用建议性记录锁改写 15.15 题图中的程序，实现父子进程的交替。
*/


#define NLOOPS  100
#define SIZE    sizeof(long)

static int update(long* ptr) {
    return((*ptr)++);
}

inline void err_sys(char* error_msg) {
    perror(error_msg);
    exit(1);
}

// 释放文件独占写锁
int release_lock(int fd) {
    struct flock lock;
    lock.l_type = F_UNLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    return fcntl(fd, F_SETLK, &lock);
}

// 请求文件独占写锁
int req_wr_lock(int fd) {
    struct flock lock;
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    return fcntl(fd, F_SETLKW, &lock);
}

int main(int argc, char** argv) {
    int fd, counter;
    pid_t pid;
    void* area;

    if ((fd = open("./lock_file", O_RDWR | O_CREAT)) < 0) 
        err_sys("open error!\n");
    // 写入0到文件中，如果文件为空，mmap()会出错
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
            if (req_wr_lock(fd) < 0) 
                err_sys("lock error!\n");
            if ((counter = update((long*) area)) != i)
                printf("parent: expected %d, got %d\n", i, counter);
            if (release_lock(fd) < 0)
                err_sys("release lock error!\n");
                sleep(1);
        }
        printf("parent counter：%d\n", counter);
        close(fd);
    }
    // 子进程
    else {
        for (int i = 1; i < NLOOPS + 1; i += 2) {
            sleep(1);
            if (req_wr_lock(fd) < 0) 
                err_sys("lock error!\n");
            if ((counter = update((long*) area)) != i)
                printf("child: expected %d, got %d\n", i, counter);
            if (release_lock(fd) < 0)
                err_sys("release lock error!\n");
        }
        printf("child counter：%d\n", counter);
        close(fd);
    }

    return 0;
}