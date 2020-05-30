#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

// 使用非阻塞写来获取管道的容量

#define err_sys(err_msg) \
    do { \
        printf(err_msg "\n");\
        exit(0);\
    } while(0)

// 更改已打开文件的状态标志
void set_fl(int fd, int flags) {
    int val;

    // 先获取原来的状态标志    
    if ((val = fcntl(fd, F_GETFL, 0)) < 0)
        err_sys("get flags error");

    // 更新状态标志
    val |= flags;

    // 保存新的状态标志
    if ((val = fcntl(fd, F_SETFL, val)) < 0)
        err_sys("set flags error");

}

int main(void) {
    int i, n;
    int fd[2];


    if (pipe(fd) < 0) {
        err_sys("pipe error");
    }

    // 将管道io设为非阻塞的
    set_fl(fd[1], O_NONBLOCK);

    // 向管道的写端fd[1]写入数据，每次写入一个字节，直到出错返回为止
    for (n = 0; ; n++) {
        if ((i = write(fd[1], "a", 1)) != 1) {
            printf("write ret %d\n", i);
            break;
        }
    }

    printf("pipe capacity = %d\n", n);
    exit(0);
}