#include<stdlib.h>
#include<stdio.h>
#include<unistd.h>

#define MAXLINE 2048

// 一个简单的管道案例

int main(void) {
    int     n;
    int     fd[2];
    pid_t   pid;
    char    line[MAXLINE];
    
    if (pipe(fd) < 0)
        printf("pipe error!\n");
    
    if ((pid = fork()) < 0) {
        printf("pipe error!\n");
    } else if (pid > 0) {
        // 在父进程中关闭管道中的输入端，也就是父进程负责将数据输出到管道中
        close(fd[0]);
        // 将数据输出到管道中
        write(fd[1], "hello world!\n", 13);
    } else {
        // 在子进程中关闭管道中的输出端，也就是子进程负责将数据从管道读出来
        close(fd[1]);
        // 将数据从管道中读出
        n = read(fd[0], line, MAXLINE);
        write(STDOUT_FILENO, line, n);
    }
    exit(0);
}
