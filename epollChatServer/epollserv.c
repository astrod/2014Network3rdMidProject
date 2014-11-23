#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>

#define BUF_SIZE 100
#define EPOLL_SIZE 50
void error_handling(char * buf);
void setNonBlocking(int fd);
void* epoll(void *arg);
void* read_data(void *arg);
void* write_data(void *arg);

int main(int argc, char *argv[])
{
    pthread_t t_epoll_id, t_cli_id, t_write_di;
    int serv_sock;
    int chat_sock;
    struct sockaddr_in serv_adr, clnt_adr, chat_adr;
    char message[BUF_SIZE];

    if(argc !=2)
    {
        printf("Usage : %s <port>\n", argv[0]);
        exit(1);
    }

    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family=AF_INET;
    serv_adr.sin_addr.s_addr=htonl(INADDR_ANY);
    serv_adr.sin_port=htons(atoi(argv[1]));

    if(bind(serv_sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr)) == -1) error_handling("bind() error");
    if(listen(serv_sock, 5) == -1) error_handling("listen() error");

    pthread_create(&t_epoll_id, NULL, epoll, &serv_sock);
    
    chat_sock = socket(PF_INET, SOCK_STREAM, 0);
    if(chat_sock == -1) error_handling("second socket() error");
    memset(&chat_adr, 0, sizeof(chat_adr));
    chat_adr.sin_family=AF_INET;
    chat_adr.sin_addr.s_addr = inet_addr("127.0.0.1");
    chat_adr.sin_port=htons(atoi(argv[1]));

    if(connect(chat_sock, (struct sockaddr*)&chat_adr, sizeof(chat_adr)) == -1) error_handling("second connect() error!");
    else
        puts("second Connected....");
    pthread_create(&t_write_di, NULL, write_data, &chat_sock);
    pthread_create(&t_cli_id, NULL, read_data, &chat_sock);
    pthread_join(t_epoll_id, NULL);
    pthread_join(t_write_di, NULL);
    pthread_join(t_cli_id, NULL);
    close(serv_sock);
    close(chat_sock);
    return 0;
}

void* epoll(void *arg)
{
    int sockArray[EPOLL_SIZE];
    int userNum = 0;
    char inputBuf[BUF_SIZE];
    socklen_t adr_sz;
    int i;
    struct epoll_event *ep_events;
    struct epoll_event event;
    int epfd, event_cnt;
    struct sockaddr_in clnt_adr;
    int clnt_sock, str_len;
    int serv_sock = *((int*)arg);

    epfd=epoll_create(EPOLL_SIZE);
    ep_events = malloc(sizeof(struct epoll_event)*EPOLL_SIZE);

    event.events = EPOLLIN;
    event.data.fd = serv_sock;
    epoll_ctl(epfd, EPOLL_CTL_ADD, serv_sock, &event);

    while(1)
    {
        event_cnt = epoll_wait(epfd, ep_events, EPOLL_SIZE, -1);
        if(event_cnt == -1)
        {
            puts("epoll_wait() error");
            break;
        }
        for(i=0; i<event_cnt; i++)
        {
            if(ep_events[i].data.fd == serv_sock)
            {
                adr_sz = sizeof(clnt_adr);
                clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_adr, &adr_sz);
                event.events = EPOLLIN;
                event.data.fd = clnt_sock;
                epoll_ctl(epfd, EPOLL_CTL_ADD, clnt_sock, &event);
                printf("connected client : %d \n", clnt_sock);
                sockArray[userNum] = clnt_sock;
                userNum++;
            }
            else
            {
                str_len = read(ep_events[i].data.fd, inputBuf, BUF_SIZE);
                if(str_len == 0)
                {
                    epoll_ctl(epfd, EPOLL_CTL_DEL, ep_events[i].data.fd, NULL);
                    close(ep_events[i].data.fd);
                    printf("closed client : %d \n", ep_events[i].data.fd);
                }
                else
                {
                    for(i =0; i<=userNum; i++) 
                    {
                        write(sockArray[i], inputBuf, strlen(inputBuf));
                    }
                    memset(inputBuf, 0, sizeof(inputBuf));
                }
            }
        }
    }
    close(epfd);
    exit(1);
}

void* write_data(void * arg)
{
    int sock = *((int*)arg);
    char message[BUF_SIZE];
    while(1)
    {
        fgets(message, BUF_SIZE, stdin);
        if(!strcmp(message, "q\n") || !strcmp(message, "Q\n"))
        {
            exit(1);
        }
        write(sock, message, strlen(message));
    }
    exit(1);
}

void* read_data(void *arg)
{
    int sock = *((int*)arg);
    char message[BUF_SIZE];
    int str_len = 0;
    while(1)
    {
        str_len = read(sock, message, BUF_SIZE-1);
        message[str_len] = 0;
        printf("%s", message);
    }
    exit(1);
}
void error_handling(char *buf)
{
    fputs(buf, stderr);
    fputc('\n', stderr);
    exit(1);
}
