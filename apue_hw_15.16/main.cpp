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

/*
使用 XSI 信号量函数改写 15.15 题图中的程序，实现父子进程的交替。
*/

#define NLOOPS  10000
#define SIZE    sizeof(long)

static int update(long* ptr) {
    return((*ptr)++);
}

inline void err_sys(char* error_msg) {
    perror(error_msg);
    exit(1);
}

inline void pv(int semo_id, int op) {
    struct sembuf buf;
    buf.sem_num = 0;
    buf.sem_op = op;
    buf.sem_flg = SEM_UNDO;
     semop(semo_id, &buf, 1);
}

int main(int argc, char** argv) {
    key_t shared_memeory_key;
    int shared_memory_id;
    int semo1_id;
    int semo2_id;
    int counter;
    void* area;
    pid_t pid;

    if ((shared_memeory_key = ftok(".", 'a')) < 0)
        err_sys("ftok error!\n");
    if ((shared_memory_id = shmget(shared_memeory_key, SIZE, IPC_CREAT|0666)) < 0)
        err_sys("shmget error!\n");
    // 直接使用IPC_PRIVATE这个key生成信号量对应的标识符
    if ((semo1_id = semget(IPC_PRIVATE, 1, IPC_CREAT|0666)) < 0)
        err_sys("semget error!\n");
    if ((semo2_id = semget(IPC_PRIVATE, 1, IPC_CREAT|0666)) < 0)
        err_sys("semget error!\n");
    // 获取指向共享内存的指针
    if ((area = shmat(shared_memory_id, 0, 0)) == (void*) -1)
        err_sys("shmat error!\n");
    // 将信号量1的初始值设置为0
    if ((semctl(semo1_id, 0, SETVAL, 0)) < 0) 
        err_sys("semctl error!\n");
    // 将信号量2的初始值设置为0
    if ((semctl(semo2_id, 0, SETVAL, 0)) < 0) 
        err_sys("semctl error!\n");

    if ((pid = fork()) < 0) {
        err_sys("fork error!\n");
    }
    // 父进程
    else if (pid > 0) {
        for (int i = 0; i < NLOOPS; i += 2) {
            
            // 获取信号量1
            pv(semo1_id, -1);
            if ((counter = update((long*) area)) != i)
                printf("parent: expected %d, got %d", i, counter);
            // 释放信号量2
            pv(semo2_id, 1);
           
        }
        printf("parent counter：%d\n", counter);
        if (waitpid(pid, NULL, 0) < 0)
            err_sys("waitpid error!\n");
        // 删除信号量
        semctl(semo1_id, 1, IPC_RMID);
        semctl(semo2_id, 1, IPC_RMID);
        // 删除共享内存
        shmctl(shared_memory_id, IPC_RMID, NULL);
    }
    // 子进程
    else {
        for (int i = 1; i < NLOOPS + 1; i += 2) {
            // 释放信号量1
            pv(semo1_id, 1);
            // 获取信号量2
            pv(semo2_id, -1);
            if ((counter = update((long*) area)) != i)
                printf("child: expected %d, got %d", i, counter);

        }
        printf("child counter：%d\n", counter);
    }
    return 0;
}