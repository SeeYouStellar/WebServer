## socket 建立连接的过程

### server端

socket()

bind()
sockaddr_in结构体
struct sockaddr_in {
    sa_family_t    sin_family; 
    in_port_t      sin_port;   
    struct in_addr sin_addr;   
};


struct in_addr {
    uint32_t       s_addr;     
};

listen()

accept()

### client端

socket()

bind()

connect()

## http请求发起到响应的过程

## 通用网关接口(CGI,Common Gateway Interface)


pthread_create

如何从一个文件描述符中读取http报文

recv 

strcasecmp : compare two strings ignoring case

get req: url包含了host以及请求内容。报文大小在1024字节范围内。
post req: url不包含请求内容，请求内容放在body中。没有报文大小的大小