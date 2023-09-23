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
#define STDIN 0
#define STDOUT 1
#define SERVER_STRING "Server: lxyhttpd/0.1.0\r\n"

// * 执行提示函数
void errorHandle(const char* _s);
void successHandle(const char* _s);
// * 数据读取函数
int getLine(int sockfd, char *buf, int size);
void rushSock(int clientfd);
// * 请求解析函数
void acceptRequest(void *clientSock);
int parseGetRequest(char* url, char* path, char* query, int usize, int psize, int qsize);
// * 服务器响应执行函数
void staticFile(int clientfd, char* method, char* filename, char* query);
void dynamicCgi(int clientfd, const char* method, const char* path, const char* query);
void header(int clientfd);
void body(int clientfd, FILE* file);
// * 错误报文响应函数
void methodUnimplemented(int client);
void badRequest(int client);
void cannotExecute(int client);
void notFound(int client);
// * socket连接函数
int startUp(int port, int backlog, int ip);

int main(void) {
    
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
void errorHandle(const char *_s) {
    perror(_s);
    exit(EXIT_FAILURE);
}
void successHandle(const char *_s) {
    printf(_s);
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
                    one = '\n';
                    // errorHandle("recv one char from http request error!");
                }
            }
            buf[n++] = one;
        }else{
            one = '\n';
            // errorHandle("recv one char from http request error!");
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
    char query[255];  // get请求path后的字符串，动态文件执行时传入的参数
    char path[255];   // 请求文件内容
    int cgi = 0;      // 请求文件是否需要动态执行
    int numchars;
    struct stat st;

    numchars = (clientfd, buf, sizeof(buf));

    
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

    // 判断method是否合法
    if(strcasecmp(method, "POST") && strcasecmp(method, "GET")){
        methodUnimplemented(clientfd);
    }else if(strcasecmp(method, "GET") == 0){
        cgi = parseGetRequest(url, path, query, sizeof(url), sizeof(path), sizeof(query));
    }else if(strcasecmp(method, "POST") == 0){
        cgi = 1;
    }   

    sprintf(path, "host%s", path); 

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
        // 未找到该资源，要把剩余的报文内容读取并丢弃，整个报文解析到此结束
        while ((numchars > 0) && strcmp("\n", buf)) // 报文结束是'\n'，head与body中间的空行是"\r\n"。所以不会中途停止
            numchars = getLine(clientfd, buf, sizeof(buf)); 
        notFound(clientfd); 
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
// ! 清空剩余报文
void rushSock(int clientfd) {
    char buf[1024];
    int numchars = 1;
    buf[0] = 'A';buf[1] = '\0';
    while(numchars>0 && strcmp("\n", buf))
        numchars = getLine(clientfd, buf, sizeof(buf));
    
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

    // 清空剩余的报文
    rushSock(clientfd);

    resource = fopen(filename, "r");
    if (resource == NULL)
        notFound(clientfd);
    else
    {
        header(clientfd);
        body(clientfd, resource);
    }
    fclose(resource);
}
/**
 * ! 动态cgi请求：
 * ? 1. 双管道传输数据
 * ? 2. 父子进程
 */
void dynamicCgi(int clientfd, const char* method, const char* path, const char* query){
    // 使用管道进行进程间通信
    int cgi_in[2], cgi_out[2];
    int status;
    char buf[512];
    int numchars;
    int content_length = -1;
    char c;
    pid_t pid;

    /**
     * ! 针对报文剩余部分，不同的method采取不同的措施
     * ? get：报文中有价值的内容path,query,method已经解析，故丢掉剩余的报文即可（get报文没有body部分，实际就是把header丢掉）
     * ? post: header中的content-length字段是body的传输长度，需要提取出来，其余内容丢掉
     */
    if(strcasecmp(method, "GET") == 0){
        rushSock(clientfd);
    }else{
        // POST 截取content-length字段
        numchars = getLine(clientfd, buf, sizeof(buf));
        // 获取HTTP消息实体的传输长度
        while ((numchars > 0) && strcmp("\n", buf)) { // 逐行判断，不是content-length的行丢弃
            buf[15] = '\0';
            if (strcasecmp(buf, "Content-Length:") == 0) 
                content_length = atoi(&(buf[16])); //读取Content-Length（描述HTTP消息实体的传输长度）
            numchars = getLine(clientfd, buf, sizeof(buf));
        }
        if (content_length == -1) {
            badRequest(clientfd); //请求页面为空，生成响应报文
            return;
        }
    }

    // 创建管道
    if(pipe(cgi_in) == -1){
        cannotExecute(clientfd);
        return ;
    }
    if(pipe(cgi_out) == -1){
        cannotExecute(clientfd);
        return ;
    }

    // 创建子进程
    pid = fork();
    if(pid == -1){
        cannotExecute(clientfd);
        return ;
    }

    sprintf(buf, "HTTP/1.0 200 OK\r\n"); //至此，动态请求成功，剩余的工作为服务器对请求的资源进行响应
    send(clientfd, buf, strlen(buf), 0); // 注意这里使用strlen而不是sizeof，sizeof会把整个buf发过去。

    if(pid == 0){
        char meth_env[255];
        char query_env[255];
        char length_env[255];
        /**
         * ? 子进程操作：
         * ! 1. 关闭cgi_in[1],cgi_out[0]
         * ! 2. 重定向标准输入STDIN -> cgi_in[0] :即所有获得输入函数（例如scanf）都从cgi_in[0]中获取数据
         * ! 3. 重定向标准输出STDOUT -> cgi_out[1] :即所有输出函数（例如printf）都输出到cgi_out[1]中
         * ! 4. 读入父进程传过来的post请求的body部分，也就是请求内容。get请求的query是共享数据，所以可以直接读取，无需父子传输
         * ! 5. 为cgi程序执行设置环境变量。执行cgi程序。程序的输出自动写入了cgi_out[1]
         * ! 6. 关闭所有管道
         * ! 7. 退出
         */
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
        execl(path, NULL);

        close(cgi_in[0]);
        close(cgi_out[1]);
        exit(0);
    }else{
        /**
         * ? 父进程操作：
         * ! 1. 关闭cgi_in[0],cgi_out[1]
         * ! 2. 如果是post请求，需要读取client socket中的剩余报文（即请求内容），注意这个报文字符数是根据content-length控制的，故需要逐个字符读取，并逐个传输到cgi_in[1]中。
         * ! 3. 读取cgi_out[0]中的cgi程序执行结果，并将结果传输到client socket中
         * ! 4. 关闭所有管道
         * ! 5. 等待子进程结束
         */
        close(cgi_in[0]);
        close(cgi_out[1]);

        if(strcasecmp(method, "POST")==0){
            for(int i=0;i<content_length;i++){
                recv(clientfd, &c, 1, 0);
                write(cgi_in[1], &c, 1);
            }
        }
        
        while((numchars = read(cgi_out[0], buf, sizeof(buf))) != 0){
            send(clientfd, buf, sizeof(buf), 0);  // 数据发送给客户端
        }

        close(cgi_in[1]);
        close(cgi_out[0]);
        waitpid(pid, &status, 0); // 等待子进程结束
    }   
}
/**
 * ! 静态文件请求 响应报文： 
 * ? header
 * ? \r\n
 * ? body
 */
void header(int clientfd) {
    char buf[512];
    
    strcpy(buf, "HTTP/1.0 200 OK\r\n");
    send(clientfd, buf, strlen(buf), 0);

    sprintf(buf, "Content-Type: text/html\r\n");
    send(clientfd, buf, strlen(buf), 0);
    strcpy(buf, "\r\n");
    send(clientfd, buf, strlen(buf), 0);
}
void body(int clientfd, FILE* file) {
    char buf[1024];
    fgets(buf, sizeof(buf), file);
    while(!feof(file)){
        send(clientfd, buf, sizeof(buf), 0);
        fgets(buf, sizeof(buf), file);
    }
}
/**
 * ! get请求的url字段：
 * ? path?query1&query2
*/
int parseGetRequest(char* url, char* path, char* query, int usize, int psize, int qsize) {
    // 解析出get请求url中的path和query
    int cgi = 0;
    int i = 0, j = 0;
    while(i < usize-1 && url[i] != '?' && j < psize-1){
        path[j++] = url[i++];
    }
    path[j] = '\0';
    if(i < usize-1 && url[i] == '?'){  // 当出现'?'时，后面的数据即为请求内容。说明这是条动态请求
        cgi = 1;  
        j = 0;
        while(i < usize-1 && url[i] != '\n' && j < qsize-1){
            query[j++] = url[i++];
        }
        query[j] = '\0';
    }
    return cgi;
}

/**
 * ! 错误处理->四种响应报文
 * ? methodUnimplemented:请求方法不存在 501
 * ? badRequest:请求出错 400
 * ? notFound:未找到请求的文件 404
 * ? badRequest:服务器执行出错 500
 */
// 请求方法不存在
void methodUnimplemented(int client){
    char buf[1024];

    sprintf(buf, "HTTP/1.0 501 Method Not Implemented\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, SERVER_STRING);
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-Type: text/html\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<HTML><HEAD><TITLE>Method Not Implemented\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "</TITLE></HEAD>\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<BODY><P>HTTP request method not supported.\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "</BODY></HTML>\r\n");
    send(client, buf, strlen(buf), 0);
}

// 请求出错
void badRequest(int client){
    char buf[1024];

    sprintf(buf, "HTTP/1.0 400 BAD REQUEST\r\n");
    send(client, buf, sizeof(buf), 0);
    sprintf(buf, "Content-type: text/html\r\n");
    send(client, buf, sizeof(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, sizeof(buf), 0);
    sprintf(buf, "<P>Your browser sent a bad request, ");
    send(client, buf, sizeof(buf), 0);
    sprintf(buf, "such as a POST without a Content-Length.\r\n");
    send(client, buf, sizeof(buf), 0);
}

// 服务器执行出错
void cannotExecute(int client) {
    char buf[1024];

    sprintf(buf, "HTTP/1.0 500 Internal Server Error\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-type: text/html\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<P>Error prohibited CGI execution.\r\n");
    send(client, buf, strlen(buf), 0);
}

// 未找到请求的文件
void notFound(int client) {
    char buf[1024];

    sprintf(buf, "HTTP/1.0 404 NOT FOUND\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, SERVER_STRING);
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-Type: text/html\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<HTML><TITLE>Not Found</TITLE>\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<BODY><P>The server could not fulfill\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "your request because the resource specified\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "is unavailable or nonexistent.\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "</BODY></HTML>\r\n");
    send(client, buf, strlen(buf), 0);
}