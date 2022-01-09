/*************************************************************************
	> File Name: server.c
	> Author: 
	> Mail: 
	> Created Time: Sat 01 Jan 2022 09:03:39 PM CST
 ************************************************************************/

#include <stdio.h>
#include <strings.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

#define SERV_PORT 8000
#define MAXLINE 80

#define prrexit(msg) { \
    perror(msg); \
    exit(1); \
}

typedef struct Task {
    int fd;
    struct Task *next;
}Task;

//任务池
typedef struct Task_pool {
    Task *head;
    Task *tail;
    pthread_mutex_t lock; 
    pthread_cond_t havetask;
}Task_pool;

//初始化任务池(任务队列)
Task_pool *task_pool_init() {
    Task_pool *tp=(Task_pool *)malloc(sizeof(Task_pool));
    tp->head=NULL;
    tp->tail=NULL;
    pthread_mutex_init(&tp->lock, NULL);  //初始化锁
    pthread_cond_init(&tp->havetask, NULL); //初始化条件变量

    return tp;
}

//增加任务(链表尾部塞入)
//参数：要塞到哪个池子里去，要塞进来数字的ID是多少
void task_pool_push(Task_pool *tp, int fd) {
    pthread_mutex_lock(&tp->lock);  //加锁

    Task *t=(Task *)malloc(sizeof(Task)); //新建一个任务,然后将这个任务加到队列中去
    t->fd=fd; 
    t->next=NULL;

    //尾插 进池子(队列)
    if(!tp->tail) {
        tp->head=tp->tail=t;
    }else {
        tp->tail->next=t;
        tp->tail=t;
    }

    pthread_cond_broadcast(&tp->havetask);
    
    pthread_mutex_unlock(&tp->lock); //解锁
}

//取出任务
//将任务从头节点中出去
Task task_pool_pop(Task_pool *tp) {
    pthread_mutex_lock(&tp->lock);  //加锁

    while(!tp->head)
        pthread_cond_wait(&tp->havetask, &tp->lock);

    Task tmp, *k;
    k=tp->head;
    tmp=*k;
    tp->head=tp->head->next;

    if(!tp->head)
        tp->tail=NULL;

    free(k);
    pthread_mutex_unlock(&tp->lock); //解锁

    return tmp;
}

//释放掉整个线程池的动态申请空间
void task_pool_free(Task_pool *tp) {
    pthread_mutex_lock(&tp->lock);  //加锁
    Task *p=tp->head;
    Task *k;

    while(p) {
        k=p;
        p=p->next;
        free(k);
    }
    tp->head=NULL;

    pthread_mutex_unlock(&tp->lock); //解锁
    pthread_mutex_destroy(&tp->lock); //释放掉锁
    pthread_cond_destroy(&tp->havetask); //释放掉条件变量
    free(tp);
    return ;
}

void *up_server(void *arg) {

    pthread_detach(pthread_self());

    int i, n;
    char buf[MAXLINE];

    Task_pool *tp=arg;

    while(1) {

        Task tmp=task_pool_pop(tp);
        int connfd=tmp.fd;
        printf("get task fd=%d\n", connfd);

        while(1) {
            n=read(connfd, buf, MAXLINE); //读取客户端传来的字符串
            if(!strncmp(buf, "quit", 4))
                break;
		
            //具体的功能实现
	    //。。。。。。。。
            
            write(connfd, buf, n);//写回给客户端
        }
        printf("finish task fd=%d\n", connfd);
        close(connfd);
    }

    return (void *)0;
}

int main() {
 
    struct sockaddr_in serveraddr, cliaddr;
    int listenfd, connfd;
    socklen_t cliaddr_len;
    char str[INET_ADDRSTRLEN];
    int i;

    Task_pool *tp=task_pool_init();

    //多线程服务器
    pthread_t tid;
    for(i=0; i<4; i++) {
        pthread_create(&tid, NULL, up_server, (void *)tp);
        printf("new thread is %#lx\n", tid);
    }



    //1. listenfd=socket();
    listenfd=socket(AF_INET, SOCK_STREAM, 0);  //IPv4， 流式通信
    if(listenfd<0) 
        prrexit("socket");
    
    //2. bind(listenfd, 服务器地址端口) -> 绑定listenfd的源地址，目标地址任意
    bzero(&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family=AF_INET;
    serveraddr.sin_port=htons(SERV_PORT);
    serveraddr.sin_addr.s_addr=htonl(INADDR_ANY);

    if(bind(listenfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr))<0)
        prrexit("bind");

    //3. listen(listenfd, 连接队列长度);  使listenfd成为一个监听描述符
    if(listen(listenfd, 2)<0)
        prrexit("listen");

    
    printf("Accepting connections...\n");
    while(1) {
        //4. connfd=accept(listenfd, 客服端地址端口); 阻塞等待客户端连接
        cliaddr_len = sizeof(cliaddr);
        connfd=accept(listenfd, (struct sockaddr *)&cliaddr, &cliaddr_len);
        if(connfd<0)
            prrexit("accept");
        
        printf("received from %s:%d\n",\
              inet_ntop(AF_INET, &cliaddr.sin_addr, str, sizeof(str)), 
              ntohs(cliaddr.sin_port));

       /* 
        * 多进程服务器
        pid_t pid=fork();
        if(pid<0)
            prrexit("fork");
        
        //父进程：等待 创建连接
        if(pid>0) {
            close(connfd);
            while(waitpid(-1, NULL, WNOHANG)>0){}
            continue;
        }
       */

        task_pool_push(tp, connfd);

    }

    task_pool_free(tp);
    
    return 0;
}
