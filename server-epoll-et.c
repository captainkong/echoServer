#include <stdio.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#define MAX_USER_COUNT 128

/**
 * 采用边沿触发的epoll模型的边沿触发模式,数据来时只会通知一次
 * 总结:数据来一次只会触发一次,这时候需要一次性将数据读完,
 * 使用while循环读.读完的现象是read出现了阻塞,但是不能阻塞,
 * 因此引入了非阻塞io,read度到最后一次返回-1,errno的值为EAGAIN,代表数据读干净.
 */

int main(int argc, char const *argv[])
{
    int lfd, cfd, epfd;
    int serverPort = 8010;
    int ret = -1;
    struct epoll_event env, clients[MAX_USER_COUNT];
    char buffer[4];
    int nready;

    // 创建套接字
    lfd = socket(AF_INET, SOCK_STREAM, 0);
    if (lfd == -1)
    {
        perror("创建套接字失败");
        return 1;
    }

    // 设置端口复用
    int opt = 1;
    setsockopt(lfd, SOL_SOCKET, SO_REUSEPORT, (void *)&opt, sizeof(opt));
    // 屏蔽信号
    signal(SIGPIPE, SIG_IGN);

    // 绑定
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(serverPort);
    addr.sin_addr.s_addr = htonl(0);
    socklen_t len = sizeof(addr);
    ret = bind(lfd, (struct sockaddr *)&addr, len);
    if (-1 == ret)
    {
        perror("绑定失败");
        return -1;
    }

    // 监听
    ret = listen(lfd, MAX_USER_COUNT);
    if (ret == -1)
    {
        perror("监听失败!");
        return -1;
    }

    // 为epoll 创建红黑树
    epfd = epoll_create(1);
    // 将监听套接字添加到红黑树中
    env.data.fd = lfd;
    env.events = EPOLLIN;
    ret = epoll_ctl(epfd, EPOLL_CTL_ADD, lfd, &env);
    if (ret == -1)
    {
        perror("上树失败");
        return -1;
    }

    // 开始监听
    while (1)
    {
        nready = epoll_wait(epfd, clients, MAX_USER_COUNT, -1);
        if (nready < 0)
        {
            perror("epoll_wait error");
            return 1;
        }
        else if (nready == 0)
        {
            continue;
        }
        for (int i = 0; i < nready; ++i)
        {
            // 判断是否是有新的客户端连接
            if (clients[i].data.fd == lfd && clients[i].events & EPOLLIN)
            {
                struct sockaddr_in cliAddr;
                socklen_t len = sizeof(cliAddr);
                cfd = accept(lfd, (struct sockaddr *)&cliAddr, &len);
                if (cfd == -1)
                {
                    perror("连接建立失败!");
                }

                // 将cfd设置为非阻塞模式
                int flags = fcntl(cfd, F_GETFL);
                flags |= O_NONBLOCK;
                fcntl(cfd, F_SETFL, flags);

                char ip[16];
                inet_ntop(AF_INET, &cliAddr.sin_addr, ip, sizeof(ip));
                printf("New Connect: %s:%d\n", ip, ntohs(cliAddr.sin_port));
                // 将新的连接添加到红黑树中
                env.data.fd = cfd;
                env.events = EPOLLIN | EPOLLET; //边沿触发
                ret = epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &env);
                if (ret == -1)
                {
                    perror("上树失败");
                    return -1;
                }
            }
            else if (clients[i].data.fd != lfd && clients[i].events & EPOLLIN)
            {
                // 处理客户端消息
                while (1)
                {
                    memset(buffer, 0, sizeof(buffer));
                    int n = read(clients[i].data.fd, buffer, sizeof(buffer) - 1);
                    buffer[strlen(buffer)] = '\0';
                    if (n < 1)
                    {
                        if (errno == EAGAIN)
                            break;
                        // 出现错误或者对方主动关闭 下树
                        close(clients[i].data.fd);
                        ret = epoll_ctl(epfd, EPOLL_CTL_DEL, clients[i].data.fd, &clients[i]);
                        printf("断开连接!\n");
                        break;
                    }

                    for (int j = 0; j < strlen(buffer); ++j)
                    {
                        if (buffer[j] >= 'a' && buffer[j] <= 'z')
                        {
                            buffer[j] -= 'a' - 'A';
                        }
                    }
                    send(clients[i].data.fd, buffer, strlen(buffer), 0);
                    if (buffer[strlen(buffer) - 1] == '\n')
                        buffer[strlen(buffer) - 1] = '\0';
                    printf("服务端转发:%s\n", buffer);
                }
            }
        }
    }

    return 0;
}
