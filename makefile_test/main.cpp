#include<stdio.h>
#include<stdlib.h>

extern void fun1();
extern void fun2();


int main() {
    printf("hello world\n");  
    fun1();  
    fun2();

    return 0;
}