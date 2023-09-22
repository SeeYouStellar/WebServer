#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/stat.h>
#include <pthread.h>  // complie with -pthread
#include <strings.h>
#include <unistd.h>
#include <sys/wait.h>

#define PORT 8080 
#define BACKLOG 10 // 允许的最大socket连接数
#define IP "172.21.251.217"
#define HOST ""
#define STDIN 0
#define STDOUT 1
int main() {
    
    int port = PORT;
    int backlog = BACKLOG;
    int ip = IP;

    // socket连接
    int serverSock = startUp(port, backlog, ip);

    struct sockaddr_in client_addr;
    socklen_t client_addrlen = sizeof(struct sockaddr_in);

    while(1){   
        successHandle("wait for http request\n");
        int clientSock = accept(serverSock, (struct sockaddr*)&client_addr, &client_addrlen); // 第二参数用来记录客户端的ip和端口号。阻塞接受。
        if(clientSock == -1){ 
            errorHandle("socket accept error!");
        }else{
            successHandle("receive a http request\n");
            pthread_t thread_id;
            if(pthread_create(&thread_id, NULL, (void*)acceptRequest, (void*)clientSock) != 0) {
                errorHandle("thread create error!");
            }else{
                successHandle("thread create success\n");
            }
        }
    }
    close(serverSock);
}
void errorHandle(const char *_s) {
    perror(_s);
    exit(EXIT_FAILURE);
}
void successHandle(const char *_s) {
    printf(_s);
}
int startUp(int port, int backlog, int ip) {
    // 建立socket
    int domain = AF_INET;
    int type = SOCK_STREAM;
    int protocol = 0;
    int sockfd = socket(domain, type, protocol);
    if(sockfd == -1) {
        errorHandle("socket build error!");
    } else{
        successHandle("socket build success!\n");
    }

    // 绑定socket地址，在指定协议中分配一个地址和端口给socket
    struct in_addr addr;
    addr.s_addr = inet_addr(ip); // 服务器程序部署在本机，故使用本机IP为socket地址
    
    struct sockaddr_in server_addr;
    socklen_t server_addrlen;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr = addr;
    server_addrlen = sizeof(struct sockaddr_in);
    if(bind(sockfd, (struct sockaddr*)&server_addr, server_addrlen) == -1){
        errorHandle("socket bind error!");
    } else{
        successHandle("socket bind success!\n");
    }

    // listen监听客户端的socket连接
    if(listen(sockfd, backlog) == -1){
        errorHandle("socket listen error!");
    } else{
        successHandle("socket listen success!\n");
    }
    
    return sockfd;
}
/*
line 格式：
[.......]\n\0
*/
int getLine(int sockfd, char *buf, int size) {
    int n = 0;
    char one = '\0';

    while(n < size-1 && one != '\n') { // 最后一个字节要存字符串结束符\0
        int ok = recv(sockfd, &one, sizeof(char), 0);  // 默认阻塞形式
        if(ok > 0){
            if(one == '\r'){ // 'r'是换行符，表示一行结束。一行结束时，http报文以\r\n结尾，需要去除后面的\n
                ok = recv(sockfd, &one, sizeof(char), MSG_PEEK); // MSG_PEEK表示读数据但不丢弃数据，后续可以再读这个数据
                if(ok > 0 && one == '\n'){
                    recv(sockfd, &one, sizeof(char), 0);
                }else{
                    errorHandle("recv one char from http request error!");
                    one = '\n';
                }
            }
            buf[n++] = one;
        }else{
            errorHandle("recv one char from http request error!");
            one = '\n';
        }
    }
    buf[n] = '\0';
    return n;
}
/*
http request:
method url httpVersion\r\n
key:value\r\n
key:value\r\n
\r\n
body
*/

/*
get req: url包含了host以及请求内容
post req: url不包含请求内容，请求内容放在body中
*/
void acceptRequest(void *clientSock) {
    int clientfd = *(int*)clientSock;
    char buf[1024];

    // 获取请求行
    char method[255];
    char url[512];
    char query[255]; // get请求内容
    char path[255];  // 请求文件内容
    char body[1024];
    int cgi = 0;
    int numCharOfLine = getline(clientfd, buf, sizeof(buf));
    struct stat st;
    // 获取method
    int i = 0, j = 0;
    while(i < sizeof(buf) && buf[i] != ' ' && j < sizeof(method)-1){
        method[j++] = buf[i++]; 
    }
    method[j] = '\0';

    // 去除空格
    while(i < sizeof(buf) && buf[i] == ' ')
        i++;

    // 获取url
    j = 0;
    while(i < sizeof(buf) && buf[i] != ' ' && j < sizeof(url)-1){
        url[j++] = buf[i++]; 
    }
    url[j] = '\0';

    // 判断method 同时判断是否是动态网页请求（cgi）
    if(strcasecmp(method, "POST") && strcasecmp(method, "GET")){
        methodUnimplemented(clientfd);
    }else if(strcasecmp(method, "GET") == 0){
        cgi = parseGetRequestURL(url, path, query, sizeof(url), sizeof(path), sizeof(query));
    }else if(strcasecmp(method, "POST") == 0){
        cgi = 1;
    }   

    sprintf(path, ".%s", path); 

    /**
     * ? 当path是目录时，默认访问index.html
     * ! path有可能最后有'/'也可能没有
     * ! 如果已经有了说明它肯定目录, 所以需要提前加上文件名index.html
     * ! 如果不是以'/'结尾，则有可能是目录，也可能不是，不能盲目判断，需要等stat函数判断
     */ 
    if (path[strlen(path) - 1] == '/') //如果url是目录则定位至该目录下index.html
        strcat(path, "index.html");

    // 查找path指向的资源
    if(stat(path, &st) == -1){
        // 未找到该资源，要把剩余的报文内容读取并丢弃
        while ((numCharOfLine > 0) && strcmp("\n", buf)) // 报文结束是'\n'，head与body中间的空行是"\r\n"。所以不会中途停止
            numCharOfLine = get_line(clientSock, buf, sizeof(buf)); //从客户端读取数据到buf
        not_found(clientSock); 
    } else {
        // 找到了资源
        if(st.st_mode & __S_IFMT == __S_IFDIR){  // 查看linux的inode节点 __S_IFMT是掩码 __S_IFDIR是目录文件
            // 发现是目录
            strcat(path, "/index.html");
        }
        /**
         * ! S_IXUSR:用户有执行权限
         * ! S_IXGRP:用户组有执行权限
         * ! S_IXOTH:其他有执行权限
         */
        if (st.st_mode & S_IXUSR || st.st_mode & S_IXGRP || st.st_mode & S_IXOTH){
            cgi = 1;
        }
        if(cgi == 1){
            dynamicCgi(clientfd, method, path, query);
        }else{
            staticFile(clientfd, method, path, query);
        }
    }
    close(clientfd);
    return ;
}
/**
 * staticFile功能：
 * ! 将可直接响应客户端的文件内容与http报文头组合形成http响应报文，然后将报文写入clientSock 
 * ! http响应报文:
 * ? 状态行
 * ? 头部字段（可省略）
 * ? \r\n
 * ? 文件内容
 * 
 * ! 状态行:
 * ? HTTP-Version Status-Code Reason-Phrase \r\n
 */
void staticFile(int clientfd, char* method, char* filename, char* query){
    
    FILE *resource = NULL;
    int numchars = 1;
    char buf[1024];

    buf[0] = 'A'; buf[1] = '\0';
    while ((numchars > 0) && strcmp("\n", buf))  // 清空请求报文的header
        numchars = get_line(clientfd, buf, sizeof(buf));

    resource = fopen(filename, "r");
    if (resource == NULL)
        not_found(clientfd);
    else
    {
        header(clientfd);
        cat(clientfd, resource);
    }
    fclose(resource);
}
void dynamicCgi(int clientfd, char* method, char* path, char* query){
    // 使用管道进行进程间通信
    int cgi_in[2], cgi_out[2];
    int status;
    char buf[512];
    int numchars;
    int content_length = -1;
    
    // todo 针对报文剩余部分，不同的method采取不同的措施

    if(strcasecmp(method, "GET") == 0){
        while ((numchars > 0) && strcmp("\n", buf))  // 清空请求报文的header
            numchars = get_line(clientfd, buf, sizeof(buf));
    }else{
        // POST 截取content-length字段
        numchars = get_line(clientfd, buf, sizeof(buf));
        //获取HTTP消息实体的传输长度
        while ((numchars > 0) && strcmp("\n", buf)) 
        {
            buf[15] = '\0';
            if (strcasecmp(buf, "Content-Length:") == 0) //是否为Content-Length字段
                content_length = atoi(&(buf[16])); //读取Content-Length（描述HTTP消息实体的传输长度）
                numchars = get_line(clientfd, buf, sizeof(buf));
            }
            if (content_length == -1) {
                bad_request(clientfd); //请求页面为空
            return;
        }
    }

    sprintf(buf, "HTTP/1.0 200 OK\r\n"); //请求成功
    send(clientfd, buf, strlen(buf), 0);

    pid_t pid;
    if(pipe(cgi_in) == -1){
        errorHandle("pipe");
    }
    if(pipe(cgi_out) == -1){
        errorHandle("pipe");
    }
    pid = fork();
    if(pid == -1){
        errorHandle("pid");
    }
    
    if(pid == 0){
        char meth_env[255];
        char query_env[255];
        char length_env[255];

        dup2(cgi_in[0], STDIN);  // 标准输入重定向到cgi_in[0]
        dup2(cgi_out[1], STDOUT);  // 标准输出重定向到cgi_out[1]
        close(cgi_in[1]);
        close(cgi_out[0]);
        
        // todo 执行cgi程序

        putenv(meth_env);
        if (strcasecmp(method, "GET") == 0) {
            //设置query_string的环境变量
            sprintf(query_env, "QUERY_STRING=%s", query); 
            putenv(query_env);
        }else {   /* POST */
        //设置content——string的环境变量
            sprintf(length_env, "CONTENT_LENGTH=%d", content_length);
            putenv(length_env);
        }
        execl(path, path, NULL);

        close(cgi_in[0]);
        close(cgi_out[1]);
        exit(0);
    }else{
        close(cgi_in[0]);
        close(cgi_out[1]);

        if(strcasecmp(method, "POST")==0){
            while((numchars = getline(clientfd, buf, sizeof(buf))) != 0){
                if(numchars == 0){
                    break;
                }
                write(cgi_in[1], buf, sizeof(buf));
            }
        }else{
            strcpy(buf, query);
            write(cgi_in[1], buf, sizeof(buf));
        }
        
        while((numchars = read(cgi_out[0], buf, sizeof(buf))) != 0){
            if(numchars == 0){
                break;
            }
            send(clientfd, buf, sizeof(buf), 0);  // 数据发送给客户端
        }


        close(cgi_in[1]);
        close(cgi_out[0]);
        waitpid(pid, &status, 0); // 等待子进程结束
    }   
}
void header(int clientfd) {
    char buf[512];
    
    strcpy(buf, "HTTP/1.0 200 OK\r\n");
    send(clientfd, buf, strlen(buf), 0);

    sprintf(buf, "Content-Type: text/html\r\n");
    send(clientfd, buf, strlen(buf), 0);
    strcpy(buf, "\r\n");
    send(clientfd, buf, strlen(buf), 0);
}
void cat(int clientfd, FILE* file) {
    char buf[1024];
    fgets(buf, sizeof(buf), file);
    while(!feof(file)){
        send(clientfd, buf, sizeof(buf), 0);
        fgets(buf, sizeof(buf), file);
    }
}
/*
get请求的url字段：
path?query1&query2

实际测试时发的url：
1. get path
2. get path?index=1
3. get path?index=2
4. post path
*/
int parseGetRequestURL(char* url, char* path, char* query, int usize, int psize, int qsize) {
    // 解析出get请求url中的path
    int cgi = 0;
    int i = 0, j = 0;
    while(i < usize-1 && url[i] != '?' && j < psize-1){
        path[j++] = url[i++];
    }
    path[j] = '\0';
    if(i < usize-1 && url[i] == '?'){  // 当出现'?'时，后面的数据即为请求内容。说明这是条动态网页请求
        cgi = 1;  
        j = 0;
        while(i < usize-1 && url[i] != '\n' && j < qsize-1){
            query[j++] = url[i++];
        }
        query[j] = '\0';
    }
    return cgi;
}


void methodUnimplemented(int client){
    
}

void bad_request(int cliendfd){

}