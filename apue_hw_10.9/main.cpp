#include <signal.h>
#include <stdio.h>

// 获取当前信号屏蔽字，并将信号屏蔽字中的每一个信号都输出

int main(int argc, char** argv) {
    sigset_t sigset;

    // 获取当前信号屏蔽字中的内容
    if (sigprocmask(0, NULL, &sigset) < 0) {
        printf("sigprocmask error!\n");
    } 

    // 遍历当前信号屏蔽字中的信号
    for (int i = 0; i < NSIG; i++) {
        if (sigismember(&sigset, i)) {
            printf("%s ", sys_siglist[i]);
        }
    }
    printf("\n");
    return 0;
}