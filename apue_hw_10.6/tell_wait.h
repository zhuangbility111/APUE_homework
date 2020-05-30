#include <signal.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

typedef void   Sigfunc(int);   /* for signal handlers */    
    
#if defined(SIG_IGN) && !defined(SIG_ERR)     
#define SIG_ERR ((Sigfunc *)-1)     
#endif

static volatile sig_atomic_t sig_flag;
static sigset_t new_mask, old_mask, zero_mask;

static void sig_usr(int signo) {
    sig_flag = 1;
}

// 环境初始化，设置信号的处理函数以及信号阻塞
void tell_wait(void) {
    if (signal(SIGUSR1, sig_usr) == SIG_ERR) {
        printf("signal(SIGUSR1) error!\n");
    }
    if (signal(SIGUSR2, sig_usr) == SIG_ERR) {
        printf("signal(SIGUSR2) error!\n");
    }
    sigemptyset(&zero_mask);
    sigemptyset(&new_mask);
    sigaddset(&new_mask, SIGUSR1);
    sigaddset(&new_mask, SIGUSR2);

    if (sigprocmask(SIG_BLOCK, &new_mask, &old_mask) < 0)
        printf("SIG_BLOCK error!\n");
}

// 唤醒父进程
void tell_parent(pid_t pid) {
    // 发送信号给父进程
    kill(pid, SIGUSR2);
}

// 等待父进程唤醒
void wait_parent(void) {
    // 阻塞，等待信号唤醒
    while (sig_flag == 0)
        sigsuspend(&zero_mask);
    sig_flag = 0;
    // if (sigprocmask(SIG_SETMASK, &old_mask, NULL) < 0)
    //     printf("SIG_SETMASK error!\n");
}

// 唤醒子进程
void tell_child(pid_t pid) {
    // 发送信号给子进程
    kill(pid, SIGUSR1);
} 

// 等待子进程唤醒
void wait_child(void) {
    while (sig_flag == 0)
        sigsuspend(&zero_mask);

    sig_flag = 0;
    // if (sigprocmask(SIG_SETMASK, &old_mask, NULL) < 0)
    //     printf("SIG_SETMASK error!\n");
}