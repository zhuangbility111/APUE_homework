
#include <stdio.h>

#include <stdlib.h>

#include <unistd.h>

#include <fcntl.h>

#include <string.h>

#include <sys/stat.h>

#include <sys/types.h>

#include <sys/wait.h>

#include <errno.h>

#include <stdarg.h>

#include <signal.h>

#include <limits.h>

#include <pthread.h>

#include <semaphore.h>

#include <sys/mman.h>

#include <sys/select.h>

#define BUFFSIZE 100

void sig_handler(int arg) {
    printf("Caught %d, %s\n", arg, strsignal(arg));
}

int main(int argc, char** argv) {

    int n;
    int written;
    char buf[BUFFSIZE];

    struct rlimit my_limit;
    // 软资源限制为1024Bytes
    my_limit.rlim_cur = 1024;
    setrlimit(RLIMIT_FSIZE, &my_limit);

    // 设置信号处理程序
    signal_intr(SIGXFSZ, sig_handler);

    while((n = read(STDIN_FILENO, buf, BUFFSIZE)) > 0) {
        if (written = write(STDOUT_FILENO, buf, n) != n) {
            fprintf(stderr, "Wrote %d bytes\n", written);
            printf("write error\n");
        }
    }

    if (n < 0)
        printf("read error\n");

    return 0;
}