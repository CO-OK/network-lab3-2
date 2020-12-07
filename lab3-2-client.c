/*
用来从客户端接受一个文件
*/
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<fcntl.h>
#include<unistd.h>
#include<sys/time.h>
#include<signal.h>
#include"msghdr.h"
#include"window.h"

int check_hdr_END(char c);
int check_hdr_SYN(char c);
int check_hdr_ACK(char c);
int check_hdr_FLG(char c);
int check_hdr_FIN(char c);
int make_dgram_client_socket();
int make_internet_address(char*,int,struct sockaddr_in*);
int check_ac(int ac,int num);
void shake_hand(int sock,struct sockaddr* saddr_to,struct sockaddr*saddr_from ,socklen_t* saddrlen);
void say_goodbye();
int make_sum( unsigned char * buf,int len);
int check_sum(unsigned char* buf,int len);
int Sendto(int fd,  void *buf, size_t n, int flags, const struct sockaddr *addr, socklen_t addr_len,int PRO);
int lab3_2_Sendto(int fd, size_t n, int flags, const struct sockaddr *addr, socklen_t addr_len,int PRO,struct WindowItem* item,int pkg_num);
int read_pkg_num(unsigned char* buf);
void make_pkg_num(unsigned char* buf,int pkg_num);
int shake_hand_state=-1;
struct itimerval time_set;
void ALRM_handler();
int set_timer(int n_msecs);

int sock;
char*msg;
struct sockaddr_in saddr_to,saddr_from;
socklen_t saddrlen;
int fd;
int main(int ac,char*av[])
{
    signal(SIGALRM,ALRM_handler);//为计时器设置好处理函数
    if(!check_ac(ac,4))
        exit(1);
    if((sock=make_dgram_client_socket())==-1)
    {
        oops("can not make sockt",2);
    }
    make_internet_address(av[1],atoi(av[2]),&saddr_to);
    msg="";
    //客户端三次握手
    shake_hand(sock,(struct sockaddr*)&saddr_to,(struct sockaddr*)&saddr_from,&saddrlen);
    if((fd=creat(av[3],0644))==-1)
        oops("create file failed",4);
    int expect_num=0;
    int nchars=0;
    unsigned char buf_recv[buf_len];
    unsigned char buf_send[buf_len];
    while(1)
    {
        nchars=recvfrom(sock,buf_recv,buf_len,0,(struct sockaddr*)&saddr_from,&saddrlen);
        shake_hand_state=3;
        printf("3333333recv pkg %d,expect=%d,chars=%d,END=%d\n",read_pkg_num(buf_recv),expect_num,nchars,check_hdr_END(buf_recv[0]));
        if(check_hdr_END(buf_recv[0])&&read_pkg_num(buf_recv)==expect_num)
        {
            make_pkg_num(buf_send,expect_num);
            Sendto(sock , buf_send, buf_len, 0, (struct sockaddr*)&saddr_to, saddrlen,END);
            break;
        }
        else if(check_sum(buf_recv,nchars)&&read_pkg_num(buf_recv)==expect_num&&!check_hdr_END(buf_recv[0]))
        {
            if((nchars-offset)!=write(fd, buf_recv+offset, nchars-offset))
                exit(8);
            make_pkg_num(buf_send,expect_num);
            Sendto(sock , buf_send, buf_len, 0, (struct sockaddr*)&saddr_to, saddrlen,ACK);
            printf("recv pkg %d,chars=%d\n",expect_num,nchars);
            printf("send pkt %d\n",expect_num);
            expect_num++;
        }
    }
    close(fd);
    return 0;
}
void shake_hand(int sock,struct sockaddr* saddr_to,struct sockaddr*saddr_from ,socklen_t* saddrlen)
{
    unsigned char buf_recv[buf_len];
    unsigned char buf_send[buf_len];
    Sendto(sock , buf_send, buf_len, 0, saddr_to, sizeof(*saddr_to),SYN);
    //设置计时器
    shake_hand_state=1;
    set_timer(1000);
    
    recvfrom(sock,buf_recv,buf_len,0,saddr_from,&(*saddrlen));
    if(check_hdr_SYN(buf_recv[0])&&check_hdr_ACK(buf_recv[0]))
    {
        Sendto(sock , buf_send, buf_len, 0, saddr_to, sizeof(*saddr_to),ACK);
        //重设计时器
        shake_hand_state=2;
        set_timer(1000);
        
    }
    else
    {
        oops("client receive a wrong msg",1);
    }
    printf("client shake done\n");
}





void say_goodbye()
{
    unsigned char buf_recv[buf_len];
    unsigned char buf_send[buf_len];
    buf_send[0]&=CLR;
    buf_send[0]|=ACK;
    buf_send[0]|=FIN;
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
    return setitimer(ITIMER_REAL,&tmp_time_set,&time_set);
}

void ALRM_handler()
{
    unsigned char buf_recv[buf_len];
    unsigned char buf_send[buf_len];
    if(shake_hand_state==1)
    {
        Sendto(sock , buf_send, buf_len, 0, (struct sockaddr*)&saddr_to, sizeof(saddr_to),SYN);
    }
    else if(shake_hand_state==2)
    {
        Sendto(sock , buf_send, buf_len, 0, (struct sockaddr*)&saddr_to, sizeof(saddr_to),ACK);
    }
}