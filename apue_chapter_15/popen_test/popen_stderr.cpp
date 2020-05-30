#include<stdlib.h>
#include<stdio.h>
#include<unistd.h>
#include<sys/wait.h>
#include<string.h>
#include<errno.h>

// 使用popen，并在父进程中将获取的输入内容输出到标准错误中

int main(int argc, char **argv) {
    FILE* fp = NULL;
    if ((fp = popen(argv[1], "r")) == NULL) {
        printf("popen error!\n");
        exit(1);
    }

    char buf[1024];
    while (fgets(buf, 1024, fp) != NULL) {
        if (fputs(buf, stderr) == EOF) {
            printf("fputs error!\n");
            exit(1);
        }
    }

    if (ferror(fp)) {
        printf("fgets error\n");
        exit(1);
    }

    return 0;
}