#include<stdio.h>
#include<stdlib.h>
#include<errno.h>
#include<sys/stat.h>
#include<fcntl.h>


// 以非阻塞的方式实现为读写而打开的FIFO

int main(int argc, char** argv) {
    // 创建fifo
    if (mkfifo(argv[1], 0644) < 0) {
        printf("mkfifo error\n");
        exit(0);
    }
    int fd;
    if ((fd = open(argv[1], O_RDWR | O_NONBLOCK)) < 0) {
        printf("open fifo error\n");
        exit(0);
    }
    return 0;
}