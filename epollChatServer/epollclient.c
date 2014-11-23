#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define BUF_SIZE 1024
void error_handling(char *message);
void* read_sock(void * arg);
void* write_sock(void * arg);
    
int main(int argc, char *argv[])
{
    pthread_t t_read_id, t_write_id;
	int sock;
	char message[BUF_SIZE];
	struct sockaddr_in serv_adr;

	if(argc!=3) 
    {
		printf("Usage : %s <IP> <port>\n", argv[0]);
		exit(1);
	}
	
	sock=socket(PF_INET, SOCK_STREAM, 0);   
	if(sock==-1)
		error_handling("socket() error");
	
	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family=AF_INET;
	serv_adr.sin_addr.s_addr=inet_addr(argv[1]);
	serv_adr.sin_port=htons(atoi(argv[2]));
	
	if(connect(sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr))==-1)
		error_handling("connect() error!");
	else
		puts("Connected...........");
	
    pthread_create(&t_read_id, NULL, read_sock, &sock);
    pthread_create(&t_write_id, NULL, write_sock, &sock);
    pthread_join(t_read_id, NULL);
    pthread_join(t_write_id, NULL);
    close(sock);
    return 0;
}

void* read_sock(void * arg)
{
    int str_len = 0;
    int sock = *((int*)arg);
    char message[BUF_SIZE];
    while(1)
    {
        str_len = read(sock, message, BUF_SIZE-1);
        message[str_len] = 0;
        printf("%s", message);
    }
}

void* write_sock(void * arg)
{
    int sock = *((int*)arg);
    char message[BUF_SIZE];
    while(1)
    {
        fgets(message, BUF_SIZE, stdin);
	    if(!strcmp(message,"q\n") || !strcmp(message,"Q\n"))
        {
            exit(1); 
        }
        write(sock, message, strlen(message));
    }
}

void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}
