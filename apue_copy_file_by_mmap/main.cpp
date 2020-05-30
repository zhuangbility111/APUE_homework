#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

// 主要是使用mmap将输入文件和输出文件映射到存储映射区中
// 然后通过内存拷贝将输入文件在映射区中的内容拷贝到输出文件在映射区中的内容
// 最后再将映射区的内容更新到文件上
// 输入文件为argv[1]，输出文件为argv[2]

#define err_sys(err_msg) \
    do { \
        printf(err_msg "\n");\
        exit(0);\
    } while(0)

#define COPYINCR (1024*1024*1024)       // 因为内存映射区大小有限，限制每一次拷贝内容的大小

int main(int argc, char** argv) {
    int         fd_in, fd_out;  // 输入输出文件的描述符
    void        *src, *dst;     // 映射区的起始地址
    size_t      copy_size;      // 每一次需要拷贝的大小
    struct stat sbuf;           // 存放文件信息结构体
    off_t       file_size = 0;  // 已拷贝内容的大小

    if (argc != 3) {
        printf("usage: %s <fromfile> <tofile>\n", argv[0]);
        exit(0);
    }

    if ((fd_in = open(argv[1], O_RDONLY)) < 0) {
        printf("can't open %s for reading\n", argv[1]);
        exit(0);
    }

    // 0666表示八进制，实际的权限位为6-6-6
    if ((fd_out = open(argv[2], O_RDWR | O_CREAT | O_TRUNC, 0666)) < 0) {
        printf("can't create %s for writting\n", argv[2]);
        exit(0);
    }

    // 获取输入文件的大小，并据此设置输出文件的大小
    if (fstat(fd_in, &sbuf) < 0) {
        err_sys("fstat error");
    }
    if (ftruncate(fd_out, sbuf.st_size) < 0) {
        err_sys("ftruncate error");
    }

    // 如果文件的总大小大于每次拷贝的上限
    // 则需要分成几次来映射、拷贝
    // 所以需要使用循环
    // file_size为已拷贝内容的大小
    while (file_size < sbuf.st_size) {
        // 获取此次拷贝内容的大小，不能超过每次拷贝的上限大小
        if (sbuf.st_size - file_size >= COPYINCR)
            copy_size = COPYINCR;
        else
            copy_size = sbuf.st_size - file_size;

        // 映射
        if ((src = mmap(0, copy_size, PROT_READ, 
            MAP_SHARED, fd_in, file_size)) == MAP_FAILED) {
            err_sys("mmap error for input");
        }
        if ((dst = mmap(0, copy_size, PROT_READ | PROT_WRITE,
            MAP_SHARED, fd_out, file_size)) == MAP_FAILED) {
                err_sys("mmap error for output");
        }

        // 复制
        memcpy(dst, src, copy_size);
        // 然后解除映射
        munmap(src, copy_size);
        // msync(dst, copy_size, MS_SYNC);
        munmap(dst, copy_size);
        // 更新已拷贝的大小
        file_size += copy_size;
    }

    exit(0);

}