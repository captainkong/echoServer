#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <signal.h>
#include <string.h>
#include <poll.h>

#define MAX_CLIENT 128

// 回显程序服务端
int main(int argc, char const *argv[])
{
    int server_port = 8010;
    int ret = -1;
    struct pollfd clients[MAX_CLIENT];
    int maxIndex = 0, nready = 0;
    char buffer[128];

    // 1 创建套接字
    int lfd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == lfd)
    {
        perror("服务端套接字建立失败");
        return 1;
    }
    // 设置套接字端口复用
    int opt = 1;
    setsockopt(lfd, SOL_SOCKET, SO_REUSEPORT, (void *)&opt, sizeof(opt));

    // 2 绑定
    struct sockaddr_in serverAddr, clientAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(server_port);
    serverAddr.sin_addr.s_addr = htonl(0);
    ret = bind(lfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
    if (-1 == ret)
    {
        perror("服务端套接字绑定失败!");
        return 1;
    }

    // 3 监听
    ret = listen(lfd, MAX_CLIENT);
    if (-1 == ret)
    {
        perror("服务端套接字监听失败!");
        return 1;
    }

    // signal(SIGPIPE, SIG_IGN);
    // 将监听套接字加入poll列表
    clients[0].fd = lfd;
    clients[0].events = POLLRDNORM;
    for (int i = 1; i < MAX_CLIENT; ++i)
    {
        clients[i].fd = -1;
        clients[i].events = POLLRDNORM;
    }

    while (1)
    {
        nready = poll((struct pollfd *)&clients, maxIndex + 1, -1);
        if (clients[0].revents & POLLRDNORM)
        {
            // 监听套接字有消息 有新的客户端连接
            socklen_t clientAddrLen = sizeof(clientAddr);
            int cfd = accept(lfd, (struct sockaddr *)&clientAddr, &clientAddrLen);
            if (cfd == -1)
            {
                perror("连接建立失败!");
            }
            char ip[16];
            inet_ntop(AF_INET, &clientAddr.sin_addr, ip, sizeof(ip));
            printf("New Connect: %s:%d\n", ip, ntohs(clientAddr.sin_port));
            for (int i = 1; i < MAX_CLIENT; ++i)
            {
                if (clients[i].fd == -1)
                {
                    clients[i].fd = cfd;
                    if (i > maxIndex)
                        maxIndex = i;
                    break;
                }
                if (i == MAX_CLIENT)
                {
                    printf("达到最大用户!\n");
                }
            }
            if (--nready == 0)
                continue;
        }
        // 处理客户端消息
        for (int i = 1; i <= maxIndex; ++i)
        {
            if (clients[i].fd == -1)
                continue;
            if (clients[i].revents & POLLRDNORM)
            {
                memset(buffer, 0, sizeof(buffer));
                ret = read(clients[i].fd, buffer, sizeof(buffer));
                if (ret < 1)
                {
                    close(clients[i].fd);
                    clients[i].fd = -1;
                    printf("断开连接!\n");
                    while (clients[maxIndex].fd == -1)
                        --maxIndex;
                    continue;
                }
                for (int j = 0; j < strlen(buffer); ++j)
                {
                    if (buffer[j] >= 'a' && buffer[j] <= 'z')
                    {
                        buffer[j] -= 'a' - 'A';
                    }
                }
                send(clients[i].fd, buffer, strlen(buffer), 0);
                if (buffer[strlen(buffer) - 1] == '\n')
                    buffer[strlen(buffer) - 1] = '\0';
                printf("服务端转发:%s\n", buffer);
                if (--nready <= 0)
                    continue;
            }
        }
    }

    return 0;
}
