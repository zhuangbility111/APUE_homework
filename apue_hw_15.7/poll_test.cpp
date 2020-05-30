#include<stdlib.h>
#include<stdio.h>
#include<unistd.h>
#include<sys/wait.h>
#include<string.h>
#include<errno.h>
#include<poll.h>

// 当一个管道被写者关闭后，解释 select 和 poll 是如何处理该管道的文件描述符的。
// 为了验证答案是否正确，编写两个小测试程序，一个用 select，另一个用 poll。
// 当一个管道的读端被关闭时，请重做此习题以查看该管道的输出描述符。
// 这个是使用poll的版本

inline void err_sys(char *err_msg) {
    printf("%s\n", err_msg);
    exit(1);
}

int main(int argc, char **argv) {
    int     fd[2];
    pid_t   pid;

    // 新建管道
    if (pipe(fd) < 0) {
        err_sys("pipe error!");
    }

    if ((pid = fork()) < 0) {
        err_sys("fork error!");
    }
    // 父进程
    else if (pid > 0) {
        // 关闭管道的读端，因为它负责写
        close(fd[0]);
        char buf[1024];
        int  n;
        strcpy(buf, "hello world!");
        if ((n = write(fd[1], buf, strlen(buf))) < 0) {
            err_sys("write error!");
        }
        sleep(5);
        printf("close fd[1]!\n");
        // 关闭写端
        close(fd[1]);
    }
    // 子进程
    else {
        // 关闭管道的写端，因为它负责读
        close(fd[1]);
        int n;
        int nfd;
        char buf[8];
        bool is_read = false;
        
 // struct pollfd {
 //     int    fd;       /* file descriptor */
 //     short  events;   /* events to look for */
 //     short  revents;  /* events returned */
 // };
        struct pollfd pf[1];
        pf[0].fd = fd[0];
        pf[0].events = POLLIN;
        for( ; ; ) {
            if ((nfd = poll(pf, 1, -1)) < 0) {
                // 被信号中断
                if (errno == EINTR)
                    continue;
                err_sys("poll error!");
            }
            else if (nfd > 0) {
                is_read = false;
                int res;
                // 如果可读，revents中对应的位会被置为1
                if ((pf[0].revents & POLLIN) != 0) {
                    printf("poll success!\n");
                    while ((n = read(fd[0], buf, sizeof(buf))) > 0) {
                        printf("length:%d, %s\n", n, buf);
                        memset(buf, 0, 8);
                        is_read = true;
                    }
                    if (n == 0 && is_read == false) {
                        printf("EOF\n");
                        break;
                    }
                }
               
                else if ((pf[0].revents & POLLHUP) != 0) {
                    printf("poll error: POLLHUP!\n");
                    break;
                }
            }
        }

    }

    return 0;


}