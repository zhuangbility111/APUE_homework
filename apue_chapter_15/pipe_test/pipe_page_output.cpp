#include<stdlib.h>
#include<stdio.h>
#include<unistd.h>
#include<sys/wait.h>
#include<string.h>

// 读入文件，并将文件的内容输出到分页程序中，实现分页输出
// 使用管道来实现
// 子进程负责运行分页程序
// 父进程负责使用fgets读入文件内容，然后借助管道将文件内容输出到在子进程中运行的分页程序
// 这也就是进程间的通信

#define DEF_PAGER "/bin/more"   // 默认的分页程序
#define MAXLINE 2048

int main(int argc, char** argv) {
    int     n;
    int     fd[2];
    pid_t   pid;
    char    *pager, *argv0;
    char    line[MAXLINE];
    FILE    *fp;

    if (argc != 2)
        return 0;
    
    // 先打开文件
    if ((fp = fopen(argv[1], "r")) == NULL) {
        printf("can't open %s\n", argv[1]);
    }
    // 建立管道
    if (pipe(fd) < 0) {
        printf("pipe error\n");
    }

    if ((pid = fork()) < 0) {
        printf("fork error\n");
    } 
    // 父进程，读入文件内容，写入到管道中
    else if (pid > 0) {
        close(fd[0]);

        while (fgets(line, MAXLINE, fp) != NULL) {
            n = strlen(line);
            if (write(fd[1], line, n) != n) {
                printf("write error to pipe\n");
            }
        }
        if (ferror(fp))
            printf("fgets error\n");
        
        close(fd[1]);

        // 等待子进程结束，获取子进程终止状态，并对子进程进行回收
        if (waitpid(pid, NULL, 0) < 0)
            printf("waitpid error\n");
        
        exit(0);
    }
    // 子进程，将标准输入换成管道输入，然后使用exec执行分页程序
    else {
        close(fd[1]);

        if (fd[0] != STDIN_FILENO) {
            // 将标准输入描述符重定向至管道输入
            if (dup2(fd[0], STDIN_FILENO) != STDIN_FILENO) {
                printf("dup2 error to stdin\n");
            }
            close(fd[0]);
        }

        // 接着为exec获取运行参数
        if ((pager = getenv("PAGER")) == NULL)
            pager = DEF_PAGER;
        if ((argv0 = strrchr(pager, '/')) != NULL)
            argv0++;
        else
            argv0 = pager;

        // 使用exec执行分页程序
        if (execl(pager, argv0, (char*)0) < 0)
            printf("execl error for %s", pager);
    }

    exit(0);

}