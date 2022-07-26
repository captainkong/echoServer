#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <signal.h>
#include <string.h>

// 回显程序服务端
int main(int argc, char const *argv[])
{
    int server_port = 8010;
    int ret = -1;
    int nready; // 加快判断
    fd_set swork, sall;
    int maxfd = 3;

    FD_ZERO(&swork);
    FD_ZERO(&sall);

    // 1 创建监听套接字
    int lfd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == lfd)
    {
        perror("服务端套接字建立失败");
        return 1;
    }
    // 设置套接字端口复用
    int opt = 1;
    setsockopt(lfd, SOL_SOCKET, SO_REUSEPORT, (void *)&opt, sizeof(opt));
    // 把监听套接字加入select集合
    FD_SET(lfd, &sall);
    maxfd = lfd;

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
    ret = listen(lfd, 8);
    if (-1 == ret)
    {
        perror("服务端套接字监听失败!");
        return 1;
    }

    signal(SIGPIPE, SIG_IGN);
    char buffer[128];

    // 4 select 轮询
    while (1)
    {
        swork = sall;
        nready = select(maxfd + 1, &swork, NULL, NULL, NULL);

        // 判断是否有新的连接
        if (FD_ISSET(lfd, &swork))
        {
            socklen_t clientAddrLen = sizeof(clientAddr);
            int cfd = accept(lfd, (struct sockaddr *)&clientAddr, &clientAddrLen);
            if (cfd == -1)
            {
                perror("连接建立失败!");
            }
            // 开始服务
            char ip[16];
            inet_ntop(AF_INET, &clientAddr.sin_addr, ip, sizeof(ip));
            printf("New Connect: %s:%d\n", ip, ntohs(clientAddr.sin_port));
            // 加入集合
            if (cfd > maxfd)
                maxfd = cfd;
            FD_SET(cfd, &sall);
            if (--nready <= 0)
                continue;

        } // end of lfd
        for (int i = lfd + 1; i <= maxfd; ++i)
        {
            if (FD_ISSET(i, &swork))
            {
                memset(buffer, 0, sizeof(buffer));
                ret = read(i, buffer, sizeof(buffer));
                if (ret < 1)
                {
                    close(i);
                    FD_CLR(i, &sall);
                    printf("断开连接!\n");
                    continue;
                }
                for (int i = 0; i < strlen(buffer); ++i)
                {
                    if (buffer[i] >= 'a' && buffer[i] <= 'z')
                    {
                        buffer[i] -= 'a' - 'A';
                    }
                }
                send(i, buffer, strlen(buffer), 0);
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
