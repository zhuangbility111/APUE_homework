#include<stdlib.h>
#include<stdio.h>
#include<unistd.h>
#include<sys/wait.h>
#include<string.h>
#include<errno.h>

// popen和pclose的实现

// 指针指向已经分配了管道的子进程数组
// 索引为文件描述符，值为子进程ID
static pid_t *child_pids = NULL;

// 能够打开的最大描述符
static int max_fd;

// cmd_string为子进程需要执行的命令，type为管道中父进程端的类型
// 返回值为父进程中连接到管道的文件指针
FILE* popen(const char* cmd_string, const char* type) {
    int     i;
    int     pfd[2];
    pid_t   pid;
    FILE    *fp;


    // 首先判断type参数是否有效
    if ((type[0] != 'r' && type[0] != 'w') || type[1] != '\0') {
        errno = EINVAL;
        return NULL;
    }

    // 如果是第一次调用，需要初始化管道-子进程数组
    // 元素的个数受到最大描述符的限制
    if (child_pids == NULL) {
        max_fd = 1024;
        if ((child_pids = (pid_t*)calloc(max_fd, sizeof(pid_t))) == NULL) {
                return NULL;
        }
    }

    // 接着就是创建管道
    if (pipe(pfd) < 0)
        return NULL;
    if (pfd[0] >= max_fd || pfd[1] >= max_fd) {
        close(pfd[0]);
        close(pfd[1]);
        errno = EMFILE;
        return NULL;
    }

    // 接着就是创建子进程
    if ((pid = fork()) < 0) {
        return NULL;
    }
    // 子进程
    else if (pid == 0) {
        // 根据父进程端在管道中的角色来决定子进程端在管道中的角色
        // 子进程端为写端
        if (type[0] == 'r') {
            close(pfd[0]);
            // 重定向子进程的标准输出至管道的写端
            if (pfd[1] != STDOUT_FILENO) {
                dup2(pfd[1], STDOUT_FILENO);
                close(pfd[1]);
            }
        }
        // 子进程端为读端
        else {
            close(pfd[1]);
            // 重定向子进程的标准输入至管道的读端
            if (pfd[0] != STDIN_FILENO) {
                dup2(pfd[0], STDIN_FILENO);
                close(pfd[0]);
            }
        }

        // 根据定义，需要关闭那些之前调用popen打开的，现在仍然在子进程中打开着的IO流
        // 所以在子进程中需要遍历child_pids数组来关闭那些仍旧打开着的描述符
        for (i = 0; i < max_fd; i++) {
            if (child_pids[i] > 0)
                close(i);
        }


        // 最后将重定向工作完成后，使用execl执行sh来解析命令
        execl("/bin/sh", "sh", "-c", cmd_string, (char*)0);
        // 只有execl执行错误才会执行下面这条语句
        _exit(127);
    }

    // 接着就是父进程进行处理
    // 决定父进程在管道中的角色，并将指向管道的文件描述符转换为文件指针，作为返回值
    if (type[0] == 'r') {
        close(pfd[1]);
        if ((fp = fdopen(pfd[0], type)) == NULL)
            return NULL;
    }
    else {
        close(pfd[0]);
        if ((fp = fdopen(pfd[1], type)) == NULL)
            return NULL;
    }

    // 最后保存子进程以及对应的文件描述符
    child_pids[fileno(fp)] = pid;
    return fp;
}

// 关闭fp指向的标准IO
// 并且获取子进程的终止状态
int pclose(FILE *fp) {
    if (fp == NULL)
        return -1;
    int fd, stat;
    pid_t pid;

    if (child_pids == NULL) {
        errno = EINVAL;
        return -1;
    }

    fd = fileno(fp);

    if (fd >= max_fd) {
        errno = EINVAL;
        return -1;
    }

    if ((pid = child_pids[fd]) == 0) {
        errno = EINVAL;
        return -1;
    }

    // 因为准备关闭管道，所以先将管道数组对应的值设为0
    child_pids[fd] = 0;
    // 关闭管道
    if (fclose(fp) == EOF) {
        return -1;
    }

    // 等待子进程结束，获取子进程的终止信息
    while (waitpid(pid, &stat, 0) < 0) {
        if (errno != EINTR)
            return -1;
    }

    return stat;
}