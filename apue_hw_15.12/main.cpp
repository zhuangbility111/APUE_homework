#include<stdlib.h>
#include<stdio.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/msg.h>
#include<string.h>

// 执行一个循环5次，在每次循环中创建一个消息队列，打印该队列的标识符，然后删除队列
// 再循环5次，在每次循环中利用键IPC_PRIVATE创建消息队列，并将一条消息放在队列中
// 程序终止后使用ipcs命令查看消息队列。解释队列标识符的变化

int main(int argc, char **argv) {
    int queue_id;
    key_t queue_key;
    for (int i = 0; i < 5; i++) {
        // 生成消息队列对应的key
        if ((queue_key = ftok(".", 'a')) < 0) {
            perror("ftok error!\n");
            exit(1);
        }
        // 根据key生成标识符，标识符用于后面操作消息队列
        if ((queue_id = msgget(queue_key, IPC_CREAT|0666)) < 0) {
            perror("msgget error!\n");
            exit(1);
        }

        printf("queue_id is %ld\n", queue_id);

        // 删除消息队列
        if (msgctl(queue_id, IPC_RMID, NULL) < 0) {
            perror("msgctl error!\n");
            exit(1);
        }

        
    }

    struct msgbuf message;
    for (int i = 0; i < 5; i++) {
        // IPC_PRIVATE这个key不需要生成，它可以直接用来生成标识符
        if ((queue_id = msgget(IPC_PRIVATE, IPC_CREAT|0666)) < 0) {
            perror("msgget error!\n");
            exit(1);
        }
        char buf[7] = "hello!";
        message.mtype = getpid();
        strcpy(message.mtext, buf);
        // 将消息发送到队列中
        if (msgsnd(queue_id, &message, sizeof(buf), IPC_NOWAIT) < 0) {
            perror("msgsnd error");
            exit(1);
        }
        else {
            printf("send successfully!\n");
        }
    }

    return 0;
    
}