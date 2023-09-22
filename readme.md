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

struct stat {
               dev_t     st_dev;         /* ID of device containing file */
               ino_t     st_ino;         /* Inode number */
               mode_t    st_mode;        /* File type and mode */
               nlink_t   st_nlink;       /* Number of hard links */
               uid_t     st_uid;         /* User ID of owner */
               gid_t     st_gid;         /* Group ID of owner */
               dev_t     st_rdev;        /* Device ID (if special file) */
               off_t     st_size;        /* Total size, in bytes */
               blksize_t st_blksize;     /* Block size for filesystem I/O */
               blkcnt_t  st_blocks;      /* Number of 512B blocks allocated */

               /* Since Linux 2.6, the kernel supports nanosecond
                  precision for the following timestamp fields.
                  For the details before Linux 2.6, see NOTES. */

               struct timespec st_atim;  /* Time of last access */
               struct timespec st_mtim;  /* Time of last modification */
               struct timespec st_ctim;  /* Time of last status change */

           #define st_atime st_atim.tv_sec      /* Backward compatibility */
           #define st_mtime st_mtim.tv_sec
           #define st_ctime st_ctim.tv_sec
           };

查看man inode

如何查看文件类型
Thus, to test for a regular file (for example), one could write:

           stat(pathname, &sb);
           if ((sb.st_mode & S_IFMT) == S_IFREG) {
               /* Handle regular file */
           }


文件读写

fopen
fgets
feof

进程间通信的方法

1. 匿名管道
2. 有名管道
3. 共享内存
fork产生的父子进程，当某个进程改变内存数据时，两个进程是不共享内存的，但是若只是读取某个数据，那么这个数据是被共享的。
4. 消息队列
5. socket
6. 信号
7. 信号量


线程间通信的方法


socket之间写/取数据
send()
receive()

读取缓冲区，写缓冲区
read()
write()

fork()
dup2()  https://blog.csdn.net/qq_28114615/article/details/94746655
历史难题


标准输入输出的重定向
