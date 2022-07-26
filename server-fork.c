#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <signal.h>
#include <string.h>

void sig_chld(int signo)
{
    pid_t pid;
    int stat;

    pid = wait(&stat);
    printf("child %d terminated\n", pid);
    return;
}

void sig_pipe(int signo)
{
}

// 回显程序服务端
int main(int argc, char const *argv[])
{
    int server_port = 8010;
    int ret = -1;

    // 1 创建套接字
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == fd)
    {
        perror("服务端套接字建立失败");
        return 1;
    }
    // 设置套接字端口复用
    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, (void *)&opt, sizeof(opt));

    // 2 绑定
    struct sockaddr_in serverAddr, clientAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(server_port);
    serverAddr.sin_addr.s_addr = htonl(0);
    ret = bind(fd, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
    if (-1 == ret)
    {
        perror("服务端套接字绑定失败!");
        return 1;
    }

    // 3 监听
    ret = listen(fd, 8);
    if (-1 == ret)
    {
        perror("服务端套接字监听失败!");
        return 1;
    }

    signal(SIGCHLD, sig_chld);
    signal(SIGPIPE, SIG_IGN);
    // struct sigaction sa;
    // sa.sa_handler = SIG_IGN;
    // sigaction(SIGPIPE, &sa, 0);

    // 4 处理新连接
    while (1)
    {
        socklen_t clientAddrLen = sizeof(clientAddr);
        ret = accept(fd, (struct sockaddr *)&clientAddr, &clientAddrLen);
        if (ret == -1)
        {
            perror("连接建立失败!");
        }

        if (0 == fork())
        {
            // 客户进程应该关闭监听套接字
            close(fd);

            // 开始服务
            char ip[16];
            inet_ntop(AF_INET, &clientAddr.sin_addr, ip, sizeof(ip));
            printf("New Connect: %s:%d\n", ip, ntohs(clientAddr.sin_port));
            char buffer[64];

            while (1)
            {
                memset(buffer, 0, sizeof(buffer));
                if (0 == read(ret, buffer, sizeof(buffer)))
                {
                    printf("断开连接: %s:%d\n", ip, ntohs(clientAddr.sin_port));
                    break;
                }
                for (int i = 0; i < strlen(buffer); ++i)
                {
                    if (buffer[i] >= 'a' && buffer[i] <= 'z')
                    {
                        buffer[i] -= 'a' - 'A';
                    }
                }
                send(ret, buffer, strlen(buffer), 0);
                if (buffer[strlen(buffer) - 1] == '\n')
                    buffer[strlen(buffer) - 1] = '\0';
                printf("服务端转发:%s\n", buffer);
            }
            close(ret);
            exit(0);
        }
        // 主进程应该关闭客户套接字
        close(ret);
    }

    return 0;
}
