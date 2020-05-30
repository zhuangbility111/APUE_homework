#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

// 实现一个可重入的putenv_r函数，也就是借助一个互斥锁实现getenv函数
// 用到了递归锁，来防止单个线程重复申请锁造成的死锁

extern char **environ;      // 环境变量二维数组

pthread_mutex_t env_mutex;      // 互斥锁，用于访问环境变量数组

static pthread_once_t init_done = PTHREAD_ONCE_INIT;        // 用于后面限制初始化函数只执行一次，防止多次初始化

// 初始化互斥锁
static void init_thread() {
    // 互斥锁属性
    pthread_mutexattr_t attr;

    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&env_mutex, &attr);
    pthread_mutexattr_destroy(&attr);

}

// string="name=value"
// 先检查环境变量表，看存在这个变量，若存在则直接改变值
// 若不存在，则在最后添加进去
int putenv_r(char* string) {
    int i;
    // 先获取name
    char* key = NULL;
    // name的长度
    int key_len;
    key = strchr(string, '=');
    if (key == NULL) {
        printf("No key!\n");
        return 0;
    }
    key_len = key - string + 1;
    pthread_once(&init_done, init_thread);
    // 访问环境变量表时先获取互斥锁
    pthread_mutex_lock(&env_mutex);
    for(i = 0; environ[i] != NULL; i++) {
        // 如果在环境变量表中找到相应的键，直接覆盖，然后返回
        // 也就是将指针直接指向string
        if (strncmp(environ[i], string, key_len) == 0) {
            environ[i] = string;
            pthread_mutex_unlock(&env_mutex);
            return 1;
        }
    }

    // 如果找不到，直接添加到环境变量表的最后
    environ[i] = string;
    environ[i+1] = NULL;
    pthread_mutex_unlock(&env_mutex);
    return 1;
}