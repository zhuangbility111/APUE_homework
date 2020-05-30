#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>

// 实现一个可重入的getenv函数，也就是借助一个互斥锁实现getenv函数
// 用到了递归锁，来防止单个线程重复申请锁造成的死锁

extern char **environ;      // 环境变量二维数组

pthread_mutex_t env_mutex;      // 互斥锁，用于访问环境变量数组

static pthread_once_t init_done = PTHREAD_ONCE_INIT;        // 用于后面限制初始化函数只执行一次，防止多次初始化



// 初始化互斥锁
static void thread_init() {
    pthread_mutexattr_t attr;       // 互斥锁属性

    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);      // 将互斥锁设置为递归锁
    pthread_mutex_init(&env_mutex, &attr);      // 将锁的属性设置到互斥锁上
    pthread_mutexattr_destroy(&attr);
}

// 根据环境变量名在环境变量表中获取相应的值
int getenv_r(const char* name, char* buf, int buflen) {
    pthread_once(&init_done, thread_init);      // 对锁进行初始化，使用这个函数保证初始化函数只执行一次，不会重复初始化锁
    int len = strlen(name);
    int i, olen;
    // 获取锁
    pthread_mutex_lock(&env_mutex);
    // 遍历环境变量表
    for (i = 0; environ[i] != NULL; i++) {
        if (strncmp(name, environ[i], len) == 0 && (environ[i][len] == '=')) {
            olen = strlen(&environ[i][len+1]);
            // 检查缓冲区长度，如果不够的话，解锁，返回错误信息
            if (olen >= buflen) {
                pthread_mutex_unlock(&env_mutex);
                return ENOSPC;
            }
            strcpy(buf, &environ[i][len+1]);
            pthread_mutex_unlock(&env_mutex);
            return 0;
        }
    }   
    pthread_mutex_unlock(&env_mutex);
    return ENOENT;
}