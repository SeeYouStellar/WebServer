#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>

#define DEST_IP "172.21.251.217" // 目标服务器IP
#define DEST_PORT 8080  // 目标服务器HOST
#define MAX_DATA 1000

int main(){
    startUp();
}

void startUp() {
    // 建立socket
    int domain = AF_INET;
    int type = SOCK_STREAM;
    int protocol = 0;
    int sockfd = socket(domain, type, protocol);
    if(sockfd == -1) {
        perror("socket build error!");
        exit(EXIT_FAILURE);
    } else{
        printf("socket build success!\n");
    }

    // 与服务器socket建立连接
    struct in_addr addr_;
    addr_.s_addr = inet_addr(DEST_IP); // 目标服务器IP

    struct sockaddr_in server_addr;
    socklen_t server_addrlen;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(DEST_PORT);
    server_addr.sin_addr = addr_;
    server_addrlen = sizeof(struct sockaddr_in);
    if(connect(sockfd, (struct sockaddr*)&server_addr, server_addrlen) == -1){
        perror("client socket connect error!");
        exit(EXIT_FAILURE);
    }else{
        printf("client socket connect success!\n");
    }

    char buf[MAX_DATA];
    int getBytesNum = recv(sockfd, buf, MAX_DATA, 0);
    if(getBytesNum == -1){
        perror("client socket recv error!");
        exit(EXIT_FAILURE);
    }else{
        printf("client socket recv %d bytes!\n", getBytesNum);
        printf("%s\n", buf);
    }
    close(sockfd);
    return ;
}
