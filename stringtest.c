#include <string.h>
#include <stdio.h>
#include <stdlib.h>

void fun(char* a, char b[512], int* c){
    printf("%d %d %d\n", sizeof(a), sizeof(b), sizeof(c));
}

int main(){
    char buf[512];
    char c = 'a';
    printf("%d\n", sizeof(c));
    printf("%d\n", sizeof(buf));
    int bufs[512];
    fun(buf, buf, bufs);
}