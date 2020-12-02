#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<string.h>
#include<unistd.h>
#include<fcntl.h>
#include<sys/time.h>
#include<signal.h>
#include<pthread.h>
#include"msghdr.h"
#include"window.h"

int check_hdr_END(char c);
int check_hdr_SYN(char c);
int check_hdr_ACK(char c);
int check_hdr_FIN(char c);
int make_dgram_server_socket(int PortNum,int QueueNum);
int get_internet_address(char*,int,int*,struct sockaddr_in*);
void say_who_called(struct sockaddr_in *);
void shake_hand();
void say_goodbye();
int make_sum( unsigned char * buf);
int check_sum(unsigned char* buf);
int Sendto(int fd,  void *buf, size_t n, int flags, const struct sockaddr *addr, socklen_t addr_len,int PRO);
void ALRM_handler();
int set_timer(int n_msecs);


int port;
int sock;
int fd;
int end;
int timer_stop;

socklen_t saddrlen;
struct ServerWindow *window;
struct sockaddr_in saddr;
struct itimerval time_set;

void  thread_send(void *arg);
void  thread_recv(void *arg);
int lab3_2_Sendto(int fd, size_t n, int flags, const struct sockaddr *addr, socklen_t addr_len,int PRO,struct WindowItem* item,int pkg_num);
int read_pkg_num(unsigned char* buf);


int main(int ac,char*av[])
{
    
    end=0;
    signal(SIGALRM,ALRM_handler);//为计时器设置好处理函数
    //检测参数是否合法
    if(ac==1||(port=atoi(av[1]))<=0)
    {
        fprintf(stderr,"usage: dgrecv portnumber\n");
        exit(1);
    }
    
    //创建服务器套接字
    if((sock=make_dgram_server_socket(port,5))==-1)
        oops("can not make socket",2);
    saddrlen=sizeof(saddr);
    
    while (1)
    {
        shake_hand();
        pthread_t threadNum_send,threadNum_recv;    
        if((pthread_create(&threadNum_send,NULL,(void*)&thread_send,(void*)(av[2])))!=0)
        {
            oops("there is somthing wrong when create a new thread\n",1);
        }
        if((pthread_create(&threadNum_recv,NULL,(void*)&thread_recv,NULL))!=0)
        {
            oops("there is somthing wrong when create a new thread\n",1);
        }
        pthread_join(threadNum_send,NULL);
        pthread_join(threadNum_recv,NULL); 
        printf("done!\n");
    }
    return 0;
}




void shake_hand()
{
     //服务器端的握手
    unsigned char buf_send[buf_len];
    unsigned char buf_recv[buf_len];
    while(1)
    {
        recvfrom(sock,buf_recv,buf_len,0,(struct sockaddr*)&saddr,&saddrlen);
        if(check_hdr_SYN(buf_recv[0]))
        {
            Sendto(sock,buf_send, buf_len, 0, (struct sockaddr*)&saddr,saddrlen,SYN|ACK);
            break;
        }
    }
    while(1)
    {
        recvfrom(sock,buf_recv,buf_len,0,(struct sockaddr*)&saddr,&saddrlen);
        if(check_hdr_ACK(buf_recv[0])&&check_sum(buf_recv))
            break;
    }
    printf("server shake done\n");
}


int set_timer(int msecs)//毫秒
{
    struct itimerval tmp_time_set;
    long sec,usec;
    sec=msecs/1000;//秒
    usec=(msecs%1000)*1000L;//微秒
    tmp_time_set.it_interval.tv_sec=sec;
    tmp_time_set.it_interval.tv_usec=usec;
    tmp_time_set.it_value.tv_sec=sec;
    tmp_time_set.it_value.tv_usec=usec;
    timer_stop=0;
    return setitimer(ITIMER_REAL,&tmp_time_set,&time_set);
}

void ALRM_handler()
{

    if(timer_stop)
        return ;
    printf("resend\n");
    struct WindowItem*tmp=window->head;
    while(tmp!=window->next_seq_num)
    {
        printf("resend %d\n",read_pkg_num(tmp->send_buf));
        lab3_2_Sendto(sock, tmp->pkg_size, 0, (struct sockaddr*)&saddr,saddrlen,tmp->send_buf[0],tmp,tmp->pkg_num);
        tmp=tmp->next;
    }
}


void thread_send(void *arg)
{
    int pkg_num=0;
    int nchars=0;
    window=malloc(sizeof(struct ServerWindow));
    MakeServerWindow(window, 5);
    fd=open((char*)arg,O_RDONLY);   
    while(1)
    {
        if(window->tail!=window->next_seq_num)
        {
            nchars=read(fd,window->next_seq_num->send_buf+offset,buf_len-offset);
            if(nchars==0)
            {
                end=0;
                lab3_2_Sendto(sock, nchars+offset, 0, (struct sockaddr*)&saddr,saddrlen,END,window->next_seq_num,pkg_num);
                printf("send end pkg %d\n",pkg_num);
                close(fd); 
                set_timer(500);
                window->next_seq_num=window->next_seq_num->next;
                printf("server send done\n");
                break;
            }
            lab3_2_Sendto(sock, nchars+offset, 0, (struct sockaddr*)&saddr,saddrlen,ACK,window->next_seq_num,pkg_num);
            printf("send pkg %d ",pkg_num);
            pkg_num++;
            if(window->head==window->next_seq_num)
            {
                set_timer(500);
            }      
            window->next_seq_num=window->next_seq_num->next;
        }
    }
}

void thread_recv(void *arg)
{
    int nchars=0;
    unsigned char recv[buf_len];
    while(1)
    {
        nchars=recvfrom(sock,recv,buf_len,0,(struct sockaddr*)&saddr,&saddrlen);
        if(check_sum(recv)&&read_pkg_num(recv)==window->head->pkg_num)
        {
            printf("recv pkg %d\n",read_pkg_num(recv));
            window->head=window->head->next;
            window->tail=window->tail->next;
            if(window->head==window->next_seq_num)
                timer_stop=1;
            if(check_hdr_END(recv[0]))
            {
                end=1;
                break;
            }
        }
    }
}

void say_goodbye()
{

}
void say_who_called(struct sockaddr_in *addrp)
{
    char    host[BUFSIZ];
    int port;
    get_internet_address(host,BUFSIZ,&port,addrp);
    printf("from: %s: %d\n",host,port);
}
