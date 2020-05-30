#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

// 先让第一个子进程对文件加读锁，持有锁之后阻塞
// 接着第二个子进程对文件加读锁
// 接着第三个子进程对文件加写锁，无法持有而阻塞
// 最后父进程对文件加读锁，看其是否能够先于第三个子进程获得锁

typedef void Sigfunc(int);

#define err_sys(err_msg) \    
    do {\
        printf(err_msg "\n");\
        exit(0);\
    } while(0)

// 对文件进行加锁的函数
int lock_reg(int fd, int cmd, int type, int whence, off_t offset, off_t len) {
    struct flock lock;

    lock.l_type = type;
    lock.l_whence = whence;
    lock.l_start = offset;
    lock.l_len = len;

    return fcntl(fd, cmd, &lock);
}

// 信号中断处理程序
void sigint(int signo) {
    
}

// 将信号与信号中断处理函数绑定，力图阻止被中断的系统调用重启动
Sigfunc* signal_intr(int signo, Sigfunc* func) {
    struct sigaction act, old_act;
    act.sa_handler = func;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    #ifdef SA_INTERRUPT
        act.sa_flags |= SA_INTERRUPT;
    #endif
    if (sigaction(signo, &act, &old_act) < 0)
        return SIG_ERR;
    return old_act.sa_handler;

} 

int main(void) {
    pid_t pid1, pid2, pid3;
    int fd;

    // 将标准输出设为不使用缓冲，也就是无缓冲，一有输出内容就马上输出
    setbuf(stdout, NULL);
    // 为SIGINT设置信号中断处理程序
    signal_intr(SIGINT, sigint);

    // 打开一个文件，有读写权限，且文件不存在时创建它
    // 第三个参数是新创建文件的权限位
    if ((fd = open("lockfile", O_RDWR | O_CREAT, 666)) < 0) {
        err_sys("can't open/create lockfile.");
    }

    // 创建子进程，并在子进程1中获取文件的读锁
    if ((pid1 = fork()) < 0) {
        err_sys("fork failed.");
    } else if (pid1 > 0) {
        // 获取文件的读锁
        if (lock_reg(fd, F_SETLK, F_RDLCK, SEEK_SET, 0, 0) < 0) {
            err_sys("child 1: can't read-lock file");
        }
        printf("child 1: obtained read lock on file\n");
        // 阻塞，一直保持持有锁的状态
        pause();
        printf("child 1: exit after pause\n");
        exit(0);
    } else {
        sleep(2);
    }

    // 创建子进程，并在子进程2中尝试获取文件的读锁
    if ((pid2 = fork()) < 0) {
        err_sys("fork failed.");
    } else if (pid2 > 0) {
        // 尝试获取文件的写锁
        if (lock_reg(fd, F_SETLK, F_RDLCK, SEEK_SET, 0, 0) < 0) {
             err_sys("child 2: can't read-lock file");
        }
        printf("child 2: obtained read lock on file\n");
        // 阻塞，一直保持持有锁的状态
        pause();
        printf("child 2: exit after pause\n");
        exit(0);
    } else {
        sleep(2);
    }

    // 创建子进程，并在子进程3中尝试获取文件的写锁
    if ((pid3 = fork()) < 0) {
        err_sys("fork failed.");
    } else if (pid3 > 0) {
        // 尝试获取写锁，无法获取不会阻塞
        if (lock_reg(fd, F_SETLK, F_WRLCK, SEEK_SET, 0, 0) < 0) {
            printf("child 3: can't set write lock: %d\n", errno);
        }
        printf("child 3 about to block in write-lock...\n");
        // 再次尝试获取写锁，此时无法获取会导致阻塞
        if (lock_reg(fd, F_SETLKW, F_WRLCK, SEEK_SET, 0, 0) < 0) {
            err_sys("child 3: can't write-lock file");
        }
        printf("child 3 returned and got write lock????\n");
        pause();
        printf("child 3: exit after pause\n");
        exit(0);
    } else {
        sleep(2);
    }

    // 在子进程3尝试获取写锁并阻塞时，最后在父进程中尝试获取写锁，看是否前面无法获取写锁会导致后面无法获得读锁
    if (lock_reg(fd, F_SETLK, F_RDLCK, SEEK_SET, 0, 0) < 0) {
        printf("child 3: can't set write lock: %d\n", errno);
    }
    else {
        printf("parent: obtained additional read lock while write lock is pending\n");
    }

    // 最后使用中断唤醒子进程，使子进程执行结束
    printf("killing child 1...\n");
    kill(pid1, SIGINT);
    printf("killing child 2...\n");
    kill(pid2, SIGINT);
    printf("killing child 3...\n");
    kill(pid3, SIGINT);
    exit(0);
}