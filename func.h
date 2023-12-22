#ifndef __FUNC__H
#define __FUNC__H

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include<sys/mman.h>

#define MAXLINE 1<<20

SSL *load_SSL(int fd);
long SSL_Read(SSL *ssl, void *buf, size_t count);
void SSL_Write(SSL *ssl, void *buf, size_t count);
void https_response(SSL *ssl, int connfd, char *filename);
void get_filetype(char *filename, char *filetype);
void find_url(char *url, char *filename);
void http_response(int connfd, char *filename);

#endif