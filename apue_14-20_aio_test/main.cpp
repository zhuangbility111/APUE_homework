#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <aio.h>

// 主要是使用异步io编程，来读入文件，并对读入内容进行翻译，然后再输出到另外一个文件
// 使用了8个异步io块，每一个异步io块负责对文件的某一部分进行读+翻译+写，
// 相当于多个异步io同时工作
// 每一个异步io块分为三个阶段：UNUSED、READ_PENDING、WRITE_PENDING
// 分别对应：空闲状态、正在读的状态、正在写的状态
// 使用轮询的方式来判断各个异步io块是否执行完毕
// 执行完毕就更新异步io块的信息，执行下一个文件块的任务


#define err_sys(err_msg) \
    do { \
        printf(err_msg "\n");\
        exit(0);\
    } while(0)

#define SIZE_PER_BLOCK 32     // 每个io块负责处理32个字节的文件内容
#define NUM_OF_BLOCK   8        // io块的个数

// io块的执行状态：空闲、正在读、正在写
enum block_status {
    UNUSED = 0,
    READ_PENDING = 1,
    WRITE_PENDING = 2
};

// io块结构体
// 包含的信息有：异步io块、当前所负责的内容、
// 所处的状态、是否是最后一个块
struct io_block {
    struct aiocb        aiocb;     // 异步io块，负责保存异步io的各种信息
    enum block_status   status;    // 所处的执行状态
    unsigned char data[SIZE_PER_BLOCK];     // 当前所负责的文件内容
    int                 last;      // 是否是最后一块
};

// io块数组
struct io_block blocks[NUM_OF_BLOCK];

// 翻译函数
unsigned char translate(unsigned char c) {
    if (isalpha(c)) {
        if (c >= 'n')
            c -= 13;
        else if (c >= 'a') 
            c += 13;
        else if (c >= 'N')
            c -= 13;
        else
            c += 13;
    }
    return c;
}

int main(int argc, char** argv) {
    if (argc != 3) 
        err_sys("usage: rot13 infile outfile");

    int i, j, fd_in, fd_out;
    int num_of_working = 0;     // 记录当前正在工作的io块的个数
    struct stat sbuf;
    off_t offset = 0;
    int err;
    size_t n;
    struct aiocb *aio_list[NUM_OF_BLOCK];   // 这个数组用于后面阻塞等待所有io块执行
    
    // 打开文件
    if ((fd_in = open(argv[1], O_RDONLY)) < 0)
        err_sys("can't open infile");
    if ((fd_out = open(argv[2], O_RDWR | O_CREAT | O_TRUNC, 0666)) < 0)
        err_sys("can't create outfile");

    // 获取输入文件的大小
    if (fstat(fd_in, &sbuf) < 0)
        err_sys("fstat failed");

    // 接着就是对io_block数组中的io_block进行初始化
    for(i = 0; i < NUM_OF_BLOCK; i++) {
        blocks[i].last = 0;
        blocks[i].status = UNUSED;
        blocks[i].aiocb.aio_buf = blocks[i].data;
        blocks[i].aiocb.aio_sigevent.sigev_notify = SIGEV_NONE;
    }

    for(;;) {
        for (i= 0; i < NUM_OF_BLOCK; i++) {
            switch (blocks[i].status) {
                // 如果当前io块属于空闲状态，则安排它进行读任务
                case UNUSED: 
                    if (offset < sbuf.st_size) {
                        blocks[i].status = READ_PENDING;
                        blocks[i].aiocb.aio_fildes = fd_in;
                        blocks[i].aiocb.aio_offset = offset;
                        offset += SIZE_PER_BLOCK;
                        if (offset >= sbuf.st_size)
                            blocks[i].last = 1;
                        blocks[i].aiocb.aio_nbytes = SIZE_PER_BLOCK;
                        if (aio_read(&blocks[i].aiocb) < 0)
                            err_sys("aio_read failed");
                        num_of_working++;
                        // 将当前io块插入阻塞队列中
                        aio_list[i] = &blocks[i].aiocb;
                    }
                    
                    break;

                // 如果当前io块处于读状态，则判断它是否读完
                // 读完则进行翻译，然后进行写任务
                // 如果没读完则跳过
                case READ_PENDING:
                    // 正在读，则跳过
                    if ((err = aio_error(&blocks[i].aiocb)) == EINPROGRESS)
                        continue;
                    if (err != 0) {
                        err_sys("aio_read failed");
                    }
                    // 读取完毕，获取执行成功后的状态
                    if ((n = aio_return(&blocks[i].aiocb)) < 0)
                        err_sys("aio_return failed");

                    // 接着就是对获取到的文件内容进行翻译，然后写出
                    if (n != SIZE_PER_BLOCK && blocks[i].last != 1)
                        err_sys("aio_read short data");
                    // 翻译
                    // for (j = 0; j < n; j++) {
                    //     blocks[i].data[j] = translate(blocks[i].data[j]);
                    // }
                    // 修改io块，将其变为写状态
                    blocks[i].status = WRITE_PENDING;
                    blocks[i].aiocb.aio_fildes = fd_out;
                    blocks[i].aiocb.aio_nbytes = n;
                    if (aio_write(&blocks[i].aiocb) < 0) {
                        err_sys("aio_write failed");
                    }

                    break;
                // 如果当前io块正处于写状态，判断其是否完成，没完成则跳过
                // 完成则将当前io块修改为空闲状态
                case WRITE_PENDING:
                    // 还未读完
                    if ((err = aio_error(&blocks[i].aiocb)) == EINPROGRESS) {
                        continue;
                    }
                    // 出错
                    if (err != 0) {
                        err_sys("aio_write failed");
                    }

                    if ((n = aio_return(&blocks[i].aiocb)) < 0) {
                       err_sys("aio_return failed");
                    }
                    if (n != blocks[i].aiocb.aio_nbytes) {
                        err_sys("aio_write short data");
                    }
                    blocks[i].status = UNUSED;
                    num_of_working--;
                    // 将当前io块从阻塞队列中移除
                    aio_list[i] = NULL;
                    break;
            }
            
        }

        // 所有io块都处于空闲状态，并且offset已经到了文件结尾，说明已经翻译完毕
        if (num_of_working == 0) {
            if (offset >= sbuf.st_size)
                break;
        } else {
            // 否则如果所有io块都处于工作状态，则阻塞，等待io块执行完毕
            if (aio_suspend(aio_list, NUM_OF_BLOCK, NULL) < 0) {
                err_sys("aio_suspend failed");
            }
        }
    }

    blocks[0].aiocb.aio_fildes = fd_out;
    if (aio_fsync(O_SYNC, &blocks[i].aiocb) < 0) {
        err_sys("aio_fsync failed");
    }

    exit(0);

}
