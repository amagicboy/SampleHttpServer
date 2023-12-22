#include "func.h"

extern int flag;

SSL *load_SSL(int fd)
{

    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();

    SSL_CTX *ctx = SSL_CTX_new(SSLv23_server_method()); // 创建服务端SSL会话环境

    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);

    if (SSL_CTX_use_certificate_file(ctx, "cnlab.cert", SSL_FILETYPE_PEM) <= 0)
    {
        // 加载公钥证书
        printf("load public key error");
        exit(1);
    }
    if (SSL_CTX_use_PrivateKey_file(ctx, "cnlab.prikey", SSL_FILETYPE_PEM) <= 0)
    {
        // 加载私钥
        printf("load private key error");
        exit(1);
    }
    if (SSL_CTX_check_private_key(ctx) <= 0)
    {
        // 检查私钥
        printf("check private key error");
        exit(1);
    }

    SSL *ssl = SSL_new(ctx);

    if (ssl == NULL)
    {
        printf("SSL_new error");
        exit(1);
    }

    if (SSL_set_fd(ssl, fd) == 0)
    {
        printf("SSL_set_fd error");
        exit(1);
    }
    return ssl;
}

long SSL_Read(SSL *ssl, void *buf, size_t count)
{
    long n;
    if ((n = SSL_read(ssl, buf, count)) < 0)
    {
        perror("read error");
        exit(1);
    }
    return n;
}
void SSL_Write(SSL *ssl, void *buf, size_t count)
{
    if (SSL_write(ssl, buf, count) < 0)
    {
        perror("write error");
        exit(1);
    }
}

void get_filetype(char *filename, char *filetype)
{
    if (strstr(filename, ".html") || strstr(filename, ".php"))
        strcpy(filetype, "text/html");
    else if (strstr(filename, ".gif"))
        strcpy(filetype, "image/gif");
    else if (strstr(filename, ".png"))
        strcpy(filetype, "image/png");
    else if (strstr(filename, ".jpg"))
        strcpy(filetype, "image/jpeg");
    else if (strstr(filename, ".mp4"))
        strcpy(filetype, "video/mp4");
    else
        strcpy(filetype, "text/plain");
}
void find_url(char *url, char *filename)
{
    char *ptr;
    if (!strstr(url, "cgi-bin"))
    {
        // 说明是静态网页
        sscanf(url, "/%s", filename);
    }
}
void https_response(SSL *ssl, int connfd, char *filename)
{
    struct stat sbuf; // 文件状态结构体
    int fd;
    char *srcp;
    char response[MAXLINE];

    bzero(response, sizeof(response));
    if (stat(filename, &sbuf) < 0)
    {
        // 文件不存在
        sprintf(response, "HTTP/1.1 404 Not Found\r");
        printf("找不到文件\n");
        SSL_Write(ssl, response, strlen(response));
    }

    else
    {
        // Send response 这是是在进行拼接
        strcat(response, "HTTP/1.0 200 OK\r\n");
        SSL_Write(ssl, response, strlen(response));

        sprintf(response, "Content-length: %ld\r\n", sbuf.st_size);
        SSL_Write(ssl, response, strlen(response));
        strcat(response, "Content-type: text/html\r\n\r\n");
        SSL_Write(ssl, response, strlen(response));

        FILE *fp = fopen(filename, "r");
        if (fp == NULL)
        {
            printf("read file error\n");
            exit(EXIT_FAILURE);
        }
        int n = 0;
        while ((n = fread(response, sizeof(char), MAXLINE, fp)) > 0)
        {
            // printf("读取到的文件长度file_block_length = %d\n", n);

            // 发送buffer中的字符串到new_server_socket,实际上就是发送给客户端
            SSL_Write(ssl, response, n);

            // 清空buffer缓存区
            bzero(response, sizeof(response));
        }
        fclose(fp);
    }
}

void http_response(int connfd, char *filename)
{
    struct stat sbuf; // 文件状态结构体
    int fd;
    char *srcp;
    char response[MAXLINE], filetype[20];

    bzero(response, sizeof(response));
    if (stat(filename, &sbuf) < 0)
    {
        // 文件不存在
        sprintf(response, "HTTP/1.1 404 Not Found\r");
        write(connfd, response, strlen(response));
    }

    else
    {
        get_filetype(filename, filetype); // 获取文件类型
        if (flag == 0)
        {
            if (filename[0] == 'd')
            {
                strcat(response, "HTTP/1.0 200 OK\r\n");
                write(connfd, response, strlen(response));
            }
            else
            {
                strcat(response, "HTTP/1.0 301 Moved Permanently\r\n");
                strcat(response, "Location: https://127.0.0.1/index.html\r\n\r\n");
                write(connfd, response, strlen(response));
            }
        }
        else if (flag == 1)
        {
            strcat(response, "HTTP/1.0 206 OK\r\n");
            write(connfd, response, strlen(response));
        }
        else if (flag == 2)
        {
            strcat(response, "HTTP/1.0 206 OK\r\n");
            write(connfd, response, strlen(response));
        }

        sprintf(response, "Content-length: %ld\r\n", sbuf.st_size);
        write(connfd, response, strlen(response));
        sprintf(response, "Content-type: %s\r\n\r\n", filetype);
        write(connfd, response, strlen(response));

        FILE *fp = fopen(filename, "r");
        if (fp == NULL)
        {
            printf("read file error\n");
            exit(EXIT_FAILURE);
        }
        int n = 0;
        if (flag == 0)
        {
            while ((n = fread(response, sizeof(char), MAXLINE, fp)) > 0)
            {
                write(connfd, response, n);
                bzero(response, sizeof(response));
            }
        }
        else if (flag == 1)
        {
            if (fseek(fp, 100, SEEK_SET) != 0)
            {
                printf("定位文件位置失败\n");
                return;
            }
            if ((n = fread(response, sizeof(char), 101, fp)) == 101)
            {
                write(connfd, response, n);
                bzero(response, sizeof(response));
            }
        }
        else if (flag == 2)
        {
            if (fseek(fp, 100, SEEK_SET) != 0)
            {
                printf("定位文件位置失败\n");
                return;
            }
            while ((n = fread(response, sizeof(char), MAXLINE, fp)) > 0)
            {
                write(connfd, response, n);
                bzero(response, sizeof(response));
            }
        }
    }
}