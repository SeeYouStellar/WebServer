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
