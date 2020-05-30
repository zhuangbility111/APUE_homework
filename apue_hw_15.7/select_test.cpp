#include<stdlib.h>
#include<stdio.h>
#include<unistd.h>
#include<sys/wait.h>
#include<string.h>
#include<errno.h>
#include<sys/select.h>

// 当一个管道被写者关闭后，解释 select 和 poll 是如何处理该管道的文件描述符的。
// 为了验证答案是否正确，编写两个小测试程序，一个用 select，另一个用 poll。
// 当一个管道的读端被关闭时，请重做此习题以查看该管道的输出描述符。
// 这个是使用select的版本

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
        sleep(3);
        printf("close fd[1]!\n");
        // 关闭写端
        close(fd[1]);
    }
    // 子进程
    else {
        // 关闭管道的写端，因为它负责读
        close(fd[1]);
        int n;
        fd_set  read_set;
        char buf[1024];
        
        for( ; ; ) {
            FD_ZERO(&read_set);
            FD_SET(fd[0], &read_set);
            if ((n = select(fd[0]+1, &read_set, NULL, NULL, NULL)) < 0) {
                err_sys("select error!");
            }
            else if (n > 0) {
                if (FD_ISSET(fd[0], &read_set)) {
                    if ((n = read(fd[0], buf, sizeof(buf))) > 0) {
                        buf[n] = '\0';
                        printf("%s\n", buf);
                    }
                    else if (n == 0) {
                        printf("EOF\n");
                        break;
                    }
                }
            }
            else {
                err_sys("time out!");
            }
        }
    }
    return 0;
}