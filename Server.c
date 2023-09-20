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
// #include <linux/in.h>

#define PORT 8080 
#define BACKLOG 10 // 允许的最大socket连接数
#define IP "172.21.251.217"
#define HOST ""
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
void* acceptRequest(void *clientSock) {
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

    // 判断method
    if(strcasecmp(method, "POST") && strcasecmp(method, "GET")){
        methodUnimplemented(clientfd);
    }else if(strcasecmp(method, "GET") == 0){
        parseGetRequestURL(url, path, query, sizeof(url), sizeof(path), sizeof(query));
    }else if(strcasecmp(method, "POST") == 0){
        cgi = 1;
    }   

    // 
}
/*
get请求的url字段：
host/path?query1&query2
*/
void parseGetRequestURL(char* url, char* path, char* query, int usize, int psize, int qsize) {
    // 解析出get请求url中的path
    int i = 0, j = 0;
    while(i < usize-1 && url[i] != '?' && j < psize-1){
        path[j++] = url[i++];
    }
    path[j] = '\0';
    j = 0;
    while(i < usize-1 && url[i] != '\n' && j < qsize-1){
        query[j++] = url[i++];
    }
    query[j] = '\0';
}
void accept_Requset(void *tclient){
    // 获取http请求，并逐行解析
    int client = *(int *)tclient;
    char buf[1024];
    int numchars;
    char method[255]; //保存请求行中的方法GET或者POST
    char url[255]; //保存请求行中的url字段
    char path[512]; //保存请求行中文件的服务器的路径
    size_t i, j;
    struct stat st;
    int cgi = 0;      /* becomes true if server decides this is a CGI
                        * program */
    char *query_string = NULL; // GET请求中？之后的查询参数

    numchars = get_line(client, buf, sizeof(buf)); //读取第一行
    i = 0; j = 0;
    while (!ISspace(buf[j]) && (i < sizeof(method) - 1)) //读取请求方法
    {
        method[i] = buf[j];
        i++; j++;
    }
    method[i] = '\0';

    if (strcasecmp(method, "GET") && strcasecmp(method, "POST")) //如果不是GET或者POST方法则返回501错误
    {
        unimplemented(client);
        return NULL;
    }

    if (strcasecmp(method, "POST") == 0)
        cgi = 1;

    i = 0;
    while (ISspace(buf[j]) && (j < sizeof(buf)))
        j++;
    while (!ISspace(buf[j]) && (i < sizeof(url) - 1) && (j < sizeof(buf))) //读取请求的url路径
    {
        url[i] = buf[j];
        i++; j++;
    }
    url[i] = '\0';

    if (strcasecmp(method, "GET") == 0)
    {
        query_string = url; //请求信息
        while ((*query_string != '?') && (*query_string != '\0')) // 截取“？”之前的字符，这之前部分为路径之后部分为参数
        query_string++;
        if (*query_string == '?') // 如果url中存在“？”，则该请求是动态请求
        {
        cgi = 1;
        *query_string = '\0';
        query_string++;
        }
    }

    //根据url拼接url在服务器上面的路径
    sprintf(path, "htdocs%s", url); 
    if (path[strlen(path) - 1] == '/') //如果url是目录则定位至该目录下index.html
        strcat(path, "index.html");
    //查找url指向的文件
    if (stat(path, &st) == -1) { //如果未找到文件
        while ((numchars > 0) && strcmp("\n", buf))  /* read & discard headers */
        numchars = get_line(client, buf, sizeof(buf)); //从客户端读取数据到buf
        not_found(client); //回应客户端未找到
    }
    else
    {
        if ((st.st_mode & S_IFMT) == S_IFDIR) //如果path为目录，则默认使用该目录下index.html文件
            strcat(path, "/index.html");
        //如果path是可执行文件，则设置cgi标志
        if ((st.st_mode & S_IXUSR) ||
            (st.st_mode & S_IXGRP) ||
            (st.st_mode & S_IXOTH)    )
            cgi = 1;
        if (!cgi) //静态页面请求
            serve_file(client, path); //直接返回文件信息
        else //动态页面请求
            execute_cgi(client, path, method, query_string); //执行cgi脚本
    }

    close(client); //关闭客户端套接字
    return NULL;
}

void methodUnimplemented(int client)
{
    
}