/*************************************************************************
	> File Name: client.c
	> Author: 
	> Mail: 
	> Created Time: Sat 01 Jan 2022 11:27:55 PM CST
 ************************************************************************/

#include <stdio.h>
#include <strings.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

#define SERV_PORT 8000
#define MAXLEN 80

int main() {

    struct sockaddr_in servaddr;

    int sockfd=socket(AF_INET, SOCK_STREAM, 0);
    char buf[MAXLEN]={"hello tcp"};


    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family=AF_INET;
    servaddr.sin_port=htons(SERV_PORT);
    inet_pton(AF_INET, "127.0.0.1", &servaddr.sin_addr);

    connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr));

    int n;
    while(n=read(0, buf, MAXLEN)) { //从屏幕上读入字符串到buf中
        write(sockfd, buf, n); //写给服务器
        if(!strncmp(buf, "quit", 4))
            break;
        n=read(sockfd, buf, MAXLEN); //服务器转换后传回给客户端，客户端接收
        printf("response from server:\n");
        write(1, buf, n);   //再打印到客户端的屏幕上
    }

    close(sockfd);

    return 0;
}
