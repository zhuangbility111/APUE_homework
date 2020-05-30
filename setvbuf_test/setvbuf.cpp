#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>
#include <cstring>

#define error(str) printf("%s",str)

void set_buf(FILE* file, char* buf) {
    struct stat st;
    int buff_size = strlen(buf);
    if (file == NULL) {
        error("File is NULL");
    }
    // 如果指定缓冲区的指针为空，或者文件类型为标准错误，说明不需要缓冲区
    if (buf == NULL || file == stderr) {
        if (setvbuf(file, buf, _IONBF, buff_size) != 0) {
            printf("setvbuf error:%s\n",strerror(errno));
        }
        else
            printf("set no buff ok!\n");
        return;
    }
    // 将FILE指针转换为文件描述符，获取文件的具体信息，根据文件的具体信息来为其设置相应的流缓冲区
    int file_fd = fileno(file);
    if (fstat(file_fd, &st) == -1)
        printf("fstat error:%s\n",strerror(errno));
    // 如果为IO文件（除了标准错误外）或者为设备文件时，缓冲区类型为行缓冲区
    if (S_ISFIFO(st.st_mode) || S_ISCHR(st.st_mode) || S_ISBLK(st.st_mode)) {
        if (setvbuf(file, buf, _IOLBF, buff_size) != 0)
            printf("setvbuf error:%s\n",strerror(errno));
        else
            printf("set line buff ok!\n");
    }
    // 如果时普通文件的话，缓冲区类型为全缓冲区
    else {
        if (setvbuf(file, buf, _IOFBF, buff_size) != 0)
            printf("setvbuf error:%s\n",strerror(errno));
        else
            printf("set full buff ok!\n");
    }
}

int main() {
    FILE *fp=fopen("input.txt","wb+");
    char buf[BUFSIZ];
    printf("%d\n",BUFSIZ);
    set_buf(fp,buf);
    set_buf(stderr,buf);
    set_buf(stdin,buf);
    set_buf(stdout,buf);
    return 0;

    
}
