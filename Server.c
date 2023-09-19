#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
// #include <linux/in.h>

#define PORT 8080 
#define BACKLOG 10 // 允许的最大socket连接数
#define IP "172.21.251.217"
int main() {
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

    // 绑定socket地址，在指定协议中分配一个地址和端口给socket
    struct in_addr addr;
    addr.s_addr = inet_addr(IP); // 服务器程序部署在本机，故使用本机IP为socket地址
    
    struct sockaddr_in server_addr;
    socklen_t server_addrlen;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr = addr;
    server_addrlen = sizeof(struct sockaddr_in);
    if(bind(sockfd, (struct sockaddr*)&server_addr, server_addrlen) == -1){
        perror("socket bind error!");
        exit(EXIT_FAILURE);
    } else{
        printf("socket bind success!\n");
    }

    // listen监听客户端的socket连接
    if(listen(sockfd, BACKLOG) == -1){
        perror("socket listen error!");
        exit(EXIT_FAILURE);
    } else{
        printf("socket listen success!\n");
    }

    struct sockaddr_in client_addr;
    socklen_t client_addrlen = sizeof(struct sockaddr_in);

    while(1){   
        printf("wait for client socket connect\n");
        int acpsockfd = accept(sockfd, (struct sockaddr*)&client_addr, &client_addrlen); // 第二参数用来记录客户端的ip和端口号。阻塞接受。
        if(acpsockfd == -1){ 
            perror("socket accept error!");
            exit(EXIT_FAILURE);
        }else{
            printf("receive success!");
            // struct msghdr msg;
            // msg.msg_name = 
            char msg[100] = "recevie success!";
            int sendBytesNume = send(acpsockfd, msg, 16, 0);
            if(sendBytesNume == -1){
                perror("socket send error!");
                exit(EXIT_FAILURE);
            } else{
                printf("socket send %d bytes!\n", sendBytesNume);
            }
            
        }
    }
    close(sockfd);
    return ;
}
