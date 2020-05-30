#include<stdlib.h>
#include<stdio.h>
#include<unistd.h>
#include<sys/wait.h>
#include<string.h>

// 功能与pipe中的程序是一样的，也就是从文件中读入内容，并通过管道将内容送到分页程序中，实现分页输出
// 使用popen代替pipe实现管道

#define PAGER "${PAGER:-more}"
#define MAXLINE 2048

int main(int argc, char** argv) {
    char    line[MAXLINE];
    FILE    *fpin, *fpout;

    if (argc != 2)
        return 0;
    
    // 打开文件
    if ((fpin = fopen(argv[1], "r")) == NULL)
        printf("can't open %s", argv[1]);

    // 使用popen创建子进程，并且建立一个管道与这个子进程相连
    if ((fpout = popen(PAGER, "w")) == NULL)
        printf("popen error\n");

    // 然后开始读文件，并将文件内容通过管道传送给子进程
    while (fgets(line, MAXLINE, fpin) != NULL) {
        // 使用fputs将内容进行输出。因为fpout已经重定向到管道的写端，因此内容会被写到管道中
        if (fputs(line, fpout) == EOF)
            printf("fputs error to pipe\n");
    }
    if (ferror(fpin))
        printf("fgets error\n");
    if (pclose(fpout) == -1)
        printf("pclose error\n");
    exit(0);
}