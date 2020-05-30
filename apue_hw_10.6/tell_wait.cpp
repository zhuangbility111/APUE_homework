#include "tell_wait.h"

int main(int argc, char **argv) {
    FILE *fp = fopen("input.txt","rw+");
    if (fp == NULL)
        printf("open file error!/n");
    pid_t pid, ppid;
    int parent_write_count = 5;
    int child_write_count = 5;
    char buf[10];
    tell_wait();
    printf("hello\n");

    // 父进程
    if ((pid = fork()) > 0) {
        while (parent_write_count--) {
            printf("wait child\n");
            wait_child();
            
            if (fgets(buf, 10, fp) != NULL) {
                int temp = atoi(buf);
                temp++;
                rewind(fp);
                fprintf(fp, "%d", temp);
                rewind(fp);
                printf("parent write file. count = %d\n", temp);
            }
            else
                printf("read file error!\n");
            printf("tell child\n");
            tell_child(pid);
        }
    } 
    else if (pid == 0) {
        // 获取父进程的pid
        ppid = getppid();
        while (child_write_count--) {
            printf("tell parent\n");
            tell_parent(ppid);
            printf("wait parent\n");
            wait_parent();
            if (fgets(buf, 10, fp) != NULL) {
                int temp = atoi(buf);
                temp++;
                rewind(fp);
                fprintf(fp, "%d", temp);
                rewind(fp);
                printf("child write file. count = %d\n", temp);
            }
            else
                printf("read file error!\n");
        }
    }
    return 0;
}