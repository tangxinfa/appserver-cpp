#include <unistd.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <assert.h>

const char* g_server_ip = "127.0.0.1";
unsigned short g_server_port = 80;
const char* g_server_host = "localhost";
int g_concurrent_connections = 1000;

int g_epoll_fd = -1;
int g_current_connections = 0;
int g_total_success = 0;
int g_total_failed = 0;

typedef struct _fd_data_t
{
    char* request;
    int request_pos;
    char* response;
    int response_pos;
}fd_data_t;

fd_data_t* g_fd_data[100000] = {'\0'};

const char* random_str(char* str, int len)
{
    const static char chars[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    memset(str, 0, len);
    for(int i = 0; i < len; ++i)
    {
        int x = rand() % (sizeof(chars) - 1);
        str[i] = chars[x];
    }
    return str;
}

void init_fd_data(int fd)
{
    if(g_fd_data[fd] == NULL)
    {
        static const char* request_format = "GET /EchoServer?msg=%s HTTP/1.1\r\nhost: %s\r\nConnection: close\r\n\r\n";
        static char msg[100];
        random_str(msg, sizeof(msg));
        static char request[1024];
        g_fd_data[fd] = (fd_data_t*)malloc(sizeof(fd_data_t));
        memset(g_fd_data[fd], 0, sizeof(fd_data_t));
        snprintf(request, sizeof(request), request_format, msg, g_server_host);
        g_fd_data[fd]->request = strdup(request);        
    }    
}

int verify_response(char* response, char* request)
{
    char* msg = strstr(response, "\r\n\r\n");
    if(msg)
    {
        msg += 8;
        char c = msg[100];
        msg[100] = '\0';
        if(strstr(request, msg))
        {
            msg[100] = c;
            return 0;
        }
        msg[100] = c;        
    }

    return 1;
}

void free_fd_data(int fd)
{
    if(g_fd_data[fd])
    {
        if(g_fd_data[fd]->response)
        {
            if(verify_response(g_fd_data[fd]->response, g_fd_data[fd]->request) == 0)
            {
                ++g_total_success;
            }
            else
            {
                ++g_total_failed;
            }
        }
        
        if(epoll_ctl(g_epoll_fd, EPOLL_CTL_DEL, fd, NULL) == -1)
        {
            perror("epoll_ctl");
        }
        close(fd);
        free(g_fd_data[fd]->request);
        free(g_fd_data[fd]->response);
        free(g_fd_data[fd]);
        g_fd_data[fd] = NULL;
        --g_current_connections;
        assert(g_current_connections >= 0);
    }
}

void async_connect()
{
    int fd = socket(PF_INET, SOCK_STREAM, 0);
    if(fd < 0)
    {
        perror("socket error");
        return;
    }

    int opt = fcntl(fd, F_GETFL);
    if(opt < 0)
    {
        perror("fcntl error");
        close(fd);
        return;
    }
    
    if(fcntl(fd, F_SETFL, opt | O_NONBLOCK) < 0)
    {
        perror("fcntl error");
        close(fd);
        return;
    }
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(g_server_port);
	if(0 == inet_aton(g_server_ip, (struct in_addr*) &addr.sin_addr.s_addr))
    {
        perror("invalid ip");
        close(fd);
        return;
    }

    struct epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLOUT|EPOLLET;
    if(epoll_ctl(g_epoll_fd, EPOLL_CTL_ADD, fd, &event) < 0)
    {
        perror("epoll_ctl error");
        close(fd);
        return;
    }

    if(connect(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1)
    {
        if(errno != EINPROGRESS)
        {
            perror("connect error");
            close(fd);
            return;
        }
    }

    ++g_current_connections;
}

int main(int argc, char *argv[])
{
    if(argc < 3)
    {
        printf("usage: %s <server ip> <server port> [concurrent connections default=%d] [host name default=%s]\n", argv[0], g_concurrent_connections, g_server_host);
        exit(EXIT_FAILURE);
    }
    
    g_server_ip = argv[1];
    g_server_port = atoi(argv[2]);
    if(argc > 3)
    {
        g_concurrent_connections = atoi(argv[3]);
    }
    if(argc > 4)
    {
        g_server_host = argv[4];
    }
    
    srand(time(NULL));

    g_epoll_fd = epoll_create(1);
    if(g_epoll_fd == -1)
    {
        perror("epoll_create error");
        exit(EXIT_FAILURE);
    }

    async_connect();

    int ready;
    struct epoll_event events[1024];
    while(ready = epoll_wait(g_epoll_fd, &events[0], sizeof(events)/sizeof(events[0]), -1))
    {
        for(int i = 0; i < ready; ++i)
        {
            if(g_fd_data[events[i].data.fd] == NULL)
            {
                init_fd_data(events[i].data.fd);
            }
        
            if(events[i].events & EPOLLOUT)
            {
                const int request_len = strlen(g_fd_data[events[i].data.fd]->request);
                int n = 0;
                while((n = send(events[i].data.fd, g_fd_data[events[i].data.fd]->request + g_fd_data[events[i].data.fd]->request_pos, request_len - g_fd_data[events[i].data.fd]->request_pos, 0)) > 0){
                    g_fd_data[events[i].data.fd]->request_pos += n;
                }
                if(n == -1)
                {
                    if(errno != EAGAIN && errno != EWOULDBLOCK){
                        perror("send error");
                        free_fd_data(events[i].data.fd);
                    }
                    continue;
                }
                if(g_fd_data[events[i].data.fd]->request_pos != request_len)
                {
                    fprintf(stderr, "request send not completion\n");
                    continue;
                }
                events[i].events = EPOLLIN|EPOLLET;
                if(epoll_ctl(g_epoll_fd, EPOLL_CTL_MOD, events[i].data.fd, &events[i]) < 0)
                {
                    perror("epoll_ctl error");
                    free_fd_data(events[i].data.fd);
                    continue;
                }
            }
            else if(events[i].events & EPOLLIN)
            {
                if(g_fd_data[events[i].data.fd]->response == NULL){
                    g_fd_data[events[i].data.fd]->response = (char*)malloc(1024);
                    memset(g_fd_data[events[i].data.fd]->response, 0, 1024);
                }            
                int n = 0;
                while((n = recv(events[i].data.fd, g_fd_data[events[i].data.fd]->response + g_fd_data[events[i].data.fd]->response_pos, 1024 - g_fd_data[events[i].data.fd]->response_pos, 0)) > 0)
                {
                    g_fd_data[events[i].data.fd]->response_pos += n;                
                }
                if(n == -1)
                {
                    if(errno != EAGAIN && errno != EWOULDBLOCK)
                    {
                        perror("recv error");
                    }
                    free_fd_data(events[i].data.fd);
                    continue;
                }
                else if(n == 0)
                {
                    free_fd_data(events[i].data.fd);
                    continue;
                }
            }
            else if(events[i].events & EPOLLERR)
            {
                free_fd_data(events[i].data.fd);
                continue;
            }
        }

        int connected = 0;
        while(g_current_connections < g_concurrent_connections)
        {
            async_connect();
            ++connected;
        }
        if(connected)
        {
            printf("%d connect, %d success, %d failed\n", g_current_connections, g_total_success, g_total_failed);
        }
    }

    return 0;
}
