#ifndef _KIT_HOOK_H_
#define _KIT_HOOK_H_

#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/uio.h>
#include <fcntl.h>
#include <sys/ioctl.h>


namespace kit_server
{



bool IsHookEnable();

void SetHookEnable(bool flag);

}

extern "C"
{

/*为了留住旧的实现方法  定义出对应的函数*/
//函数指针
//sleep
typedef unsigned int (*sleep_func)(unsigned int secends);
extern sleep_func sleep_f;

//usleep
typedef int (*usleep_func)(useconds_t usec);
extern usleep_func usleep_f;

//nanosleep
typedef int (*nanosleep_func)(const struct timespec *req, struct timespec *rem);
extern nanosleep_func nanosleep_f;


/**********************************socket***********************************/
//socket
typedef int (*socket_func)(int domain, int type, int protocol);
extern socket_func socket_f;

//超时 connect
extern int connect_with_timeout(int sockfd, const struct sockaddr *addr, socklen_t addrlen, uint64_t timeout_ms);

//connect
typedef int (*connect_func)(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
extern connect_func connect_f;

//accept
typedef int (*accept_func)(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
extern accept_func accept_f;

//close
typedef int (*close_func)(int fd);
extern close_func close_f;

//fcntl
typedef int (*fcntl_func)(int fd, int cmd, ... /* arg */ );
extern fcntl_func fcntl_f;

//ioctl
typedef int (*ioctl_func)(int fd, unsigned long request, ...);
extern ioctl_func ioctl_f;

//getsockopt
typedef int (*getsockopt_func)(int sockfd, int level, int optname, void *optval, socklen_t *optlen);
extern getsockopt_func getsockopt_f;

//setsockopt
typedef int (*setsockopt_func)(int sockfd, int level, int optname, const void *optval, socklen_t optlen);
extern setsockopt_func setsockopt_f;


/************************************读相关*********************************/
//read
typedef ssize_t (*read_func)(int fd, void *buf, size_t count);
extern read_func read_f;

//readv
typedef ssize_t (*readv_func)(int fd, const struct iovec *iov, int iovcnt);
extern readv_func readv_f;

//recv
typedef ssize_t (*recv_func)(int sockfd, void *buf, size_t len, int flags);
extern recv_func recv_f;

//recvfrom
typedef ssize_t (*recvfrom_func)(int sockfd, void *buf, size_t len, int flags, 
    struct sockaddr *src_addr, socklen_t *addrlen);
extern recvfrom_func recvfrom_f;

//recvmsg
typedef ssize_t (*recvmsg_func)(int sockfd, struct msghdr *msg, int flags);
extern recvmsg_func recvmsg_f;


/**********************************写相关***********************************/

//write
typedef ssize_t (*write_func)(int fd, const void *buf, size_t count);
extern write_func write_f;

//writev
typedef ssize_t (*writev_func)(int fd, const struct iovec *iov, int iovcnt);
extern writev_func writev_f;

//send
typedef ssize_t (*send_func)(int sockfd, const void *buf, size_t len, int flags);
extern send_func send_f;

//sendto
typedef ssize_t (*sendto_func)(int sockfd, const void *buf, size_t len, int flags,
    const struct sockaddr *dest_addr, socklen_t addrlen);
extern sendto_func sendto_f;
 
//sendmsg
typedef ssize_t (*sendmsg_func)(int sockfd, const struct msghdr *msg, int flags);
extern sendmsg_func sendmsg_f;

}


#endif 