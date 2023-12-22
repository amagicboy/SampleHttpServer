#include <sys/types.h> /* See NOTES */
#include <sys/socket.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include "func.h"

#define MAXFD 100

int flag = 0;

int gSocketFd80 = 0, gSocketFd443 = 0;
pthread_t tid1, tid2;

int socket_init(int port)
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        return -1;
    }
    struct sockaddr_in saddr;
    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(port);
    saddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    int res = bind(sockfd, (struct sockaddr *)&saddr, sizeof(saddr));
    if (res == -1)
    {
        printf("bind err\n");
        return -1;
    }
    res = listen(sockfd, 5);
    if (res == -1)
    {
        return -1;
    }
    return sockfd;
}

void *thread_func1(void *arg)
{
    struct sockaddr_in cliaddr;
    socklen_t cliaddr_len;
    int n;
    int client;
    char buff[MAXLINE], first_line[MAXLINE], left_line[MAXLINE], method[MAXLINE], url[MAXLINE], version[MAXLINE];
    char filename[MAXLINE];
    char response[MAXLINE];
    int on = 1;
    while (1)
    {
        cliaddr_len = sizeof(cliaddr);
        client = accept(gSocketFd80, (struct sockaddr *)&cliaddr, &cliaddr_len);

        memset(buff, 0, sizeof(buff));
        memset(method, 0, sizeof(method));
        memset(url, 0, sizeof(url));
        memset(version, 0, sizeof(version));
        memset(filename, 0, sizeof(filename));
        memset(response, 0, sizeof(response));

        setsockopt(client, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
        n = read(client, buff, MAXLINE);
        if (n == 0)
        {
            printf("the other side has been closed.\n");
            break;
        }

        sscanf(buff, "%s %s %s", method, url, version);
        find_url(url, filename); // 解析url，获取文件名

        char str2[] = "bytes=";
        char *ptr = strstr(buff, str2);
        flag=0;
        if (ptr != NULL)
        {
            if(buff[165]&&buff[165]=='2')flag=1;
            else flag=2;
        }
        http_response(client, filename);
        close(client);
    }
    
}


void *thread_func2(void *arg)
{
    struct sockaddr_in cliaddr;
    socklen_t cliaddr_len;
    int n;
    int client;
    char buff[MAXLINE], first_line[MAXLINE], left_line[MAXLINE], method[MAXLINE], url[MAXLINE], version[MAXLINE];
    char filename[MAXLINE];
    char response[MAXLINE];
    int on = 1;
    SSL *ssl;
    while (1)
    {
        cliaddr_len = sizeof(cliaddr);
        client = accept(gSocketFd443, (struct sockaddr *)&cliaddr, &cliaddr_len);

        memset(buff, 0, sizeof(buff));
        memset(method, 0, sizeof(method));
        memset(url, 0, sizeof(url));
        memset(version, 0, sizeof(version));
        memset(filename, 0, sizeof(filename));
        memset(response, 0, sizeof(response));

        setsockopt(client, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
        ssl = load_SSL(client);
        if (SSL_accept(ssl) == -1)
        {
            ERR_print_errors_fp(stderr);
        }

        int n = SSL_Read(ssl, buff, MAXLINE);
        if (n == 0)
        {
            printf("the other side has been closed.\n");
            break;
        }

        sscanf(buff, "%s %s %s", method, url, version);

        find_url(url, filename);               // 解析url，获取文件名
        https_response(ssl, client, filename); // 响应客户端
        close(client);
        SSL_free(ssl);
    }
}

int main()
{
    int err;

    gSocketFd80 = socket_init(80); // 调用创建套接字函数
    if (gSocketFd80 == -1)
    {
        exit(0);
    }
    gSocketFd443 = socket_init(443); // 调用创建套接字函数
    if (gSocketFd443 == -1)
    {
        exit(0);
    }

    err = pthread_create(&tid1, NULL, thread_func1, NULL);
    err = pthread_create(&tid1, NULL, thread_func2, NULL);

    while (1)
        ;
    return 0;
}