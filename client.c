#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>

// 回显程序客户端
int main(int argc, char const *argv[])
{
    int ret = -1;

    // 创建套接字
    int confd = socket(AF_INET, SOCK_STREAM, 0);

    // 设置服务端地址
    struct sockaddr_in serAddr;
    bzero(&serAddr, sizeof(serAddr));
    inet_pton(AF_INET, "127.0.0.1", &serAddr.sin_addr);
    serAddr.sin_port = htons(8010);
    serAddr.sin_family = AF_INET;

    // 连接到服务端
    ret = connect(confd, (struct sockaddr *)&serAddr, sizeof(serAddr));
    if (-1 == ret)
    {
        perror("连接到服务端失败");
        return 1;
    }

    char buffer[128];
    while (1)
    {
        memset(buffer, 0, sizeof(buffer));
        scanf("%s", buffer);
        send(confd, buffer, strlen(buffer), 0);
        memset(buffer, 0, sizeof(buffer));
        if (0 == read(confd, buffer, sizeof(buffer)))
        {
            printf("断开与服务端断开连接!\n");
            break;
        }
        printf("%s\n", buffer);
    }

    return 0;
}
