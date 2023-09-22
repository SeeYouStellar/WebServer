#include <stdio.h>
#include <pthread.h>  // complie with -pthread
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <fcntl.h>
int main(){
    int pipe1[2], pipe2[2];
    pid_t pid;
    char buf[255];
    char get[255];
    if(pipe(pipe1) == -1){
        perror("pipe");
        exit(EXIT_FAILURE);
    }
    if(pipe(pipe2) == -1){
        perror("pipe");
        exit(EXIT_FAILURE);
    }
    pid=fork();
    if(pid == -1){
        perror("fork");
        exit(EXIT_FAILURE);
    }
    if(pid == 0){ // 子进程
        close(pipe1[0]);
        close(pipe2[1]);
        int fd = open("test", O_RDWR);
        if(fd == -1){
            perror("file");
            exit(-1);
        }
        dup2(pipe2[0], 0);
        dup2(fd, 1);
        // read(pipe2[0], buf, sizeof(buf));
        scanf("%s", buf);
        printf("%s\n", buf);

        close(pipe1[1]);
        close(pipe2[0]);
    }else{  // 父进程
        close(pipe1[1]);
        close(pipe2[0]);
        // sleep(10);
        strcpy(buf, "第一条数据");
        write(pipe2[1], buf, sizeof(buf));
        close(pipe1[0]);
        close(pipe2[1]);
    }
}