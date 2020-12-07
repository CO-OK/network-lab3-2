#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<netdb.h>
#include<string.h>
#include"msghdr.h"
#include"window.h"
#define HOSTLEN     256
#ifndef oops
#define oops(m,x)   {perror(m)  ;exit(x)    ;}
#endif
//int make_internet_address();

int make_dgram_server_socket(int PortNum,int QueueNum)
{
    /*
        封装建立服务器的一些系统调用
        PortNum：端口
        QueueNum 队列长
    */
    struct sockaddr_in saddr;
    int sock_id;
    if((sock_id=socket(PF_INET,SOCK_DGRAM,0))==-1)
    {
        oops("There is somthing wrong when server make a sock\n",5);
    }
    bzero((void*)&saddr,sizeof(saddr));
    saddr.sin_addr.s_addr=htons(INADDR_ANY);
    saddr.sin_port=htons(PortNum);//端口号
    saddr.sin_family=AF_INET;//地址族
    int opt = 1;
    if(bind(sock_id,(struct sockaddr*)&saddr,sizeof(saddr))!=0)//将sock与saddr绑定
    {
        oops("There is somthing wrong when server bind a sock\n",5);
    }
    return sock_id;
}
int get_internet_address(char*host,int len,int*portp,struct sockaddr_in *addrp)
{
    //获得host对应的网络相关信息
    strncpy(host,inet_ntoa(addrp->sin_addr),len);
    *portp=ntohs(addrp->sin_port);
    return 0;
}

int make_dgram_client_socket()
{
    //创建客户端的套接字
    return socket(PF_INET,SOCK_DGRAM,0);
}

int make_internet_address(char*ip,int port,struct sockaddr_in *addrp)
{
    /*
        将ip、port填进addrp
    */
    bzero((void*)addrp,sizeof(*addrp));//先清空saddr
    addrp->sin_addr.s_addr=inet_addr(ip);
    addrp->sin_port=htons(port);
    addrp->sin_family=AF_INET;
    return 0;
}

int check_ac(int ac,int num)
{
    if(ac==num)
        return 1;
    else
    {
        fprintf(stderr,"agr number is not correct\n");
        return 0;
    }
}

void clear_buf(char*buf)
{
    for(int i=0;i<BUFSIZ+1;i++)
        buf[i]='\0';
}

void delay(unsigned i)
{
    //延时一段时间，不过现在用usleep()代替
    for(int j=0;j<i;j++);
}

int check_sum( unsigned char* buf,int len)
{
    /*
        检查buf的校验和，如果通过，则返回1否则返回0
    */
    int sum=0;
    for(int i=0;i<len;i++)
    {
        if(i!=1&&i!=2&&i!=3)
        {
            sum+=buf[i];
        }
    }
    unsigned char a,b,c;
    a=sum;
    b=sum>>8;
    c=sum>>16;
    if(a+buf[1]==255&&b+buf[2]==255&&c+buf[3]==255)
        return 1;
    return 0;
}

int make_sum(unsigned char * buf,int len)
{
    /*
        制作长为len的buf的校验和存在第1、2、3位
    */
    int sum=0;
    for(int i=0;i<len;i++)
    {
        //printf("%d\n",buf[i]);
        if(i!=1&&i!=2&&i!=3)
            sum+=buf[i];
        else
        {
            buf[i]=0;
        }
    }
    buf[1]=~sum;
    buf[2]=~(sum>>8);
    buf[3]=~(sum>>16);
    return sum;
}

void make_hdr(unsigned char*buf,int PRO)
{
    /*
    根据PRO，将相应的属性位置1
    */
    buf[0]&=CLR;
    buf[0]|=PRO;
}


int Sendto(int fd,  void *buf, size_t n, int flags, const struct sockaddr *addr, socklen_t addr_len,int PRO)
{
    /*
        集成make_hdr make_sum sendto
    */
    make_hdr(buf,PRO);
    make_sum(buf,n);
    return sendto(fd,buf,n,flags,addr,addr_len);
}


/*
检查相应属性位是否为1,是1则返回1
*/
int check_hdr_SYN(char c)
{
    return ((c&SYN)==64);
}
int check_hdr_ACK(char c)
{
    return ((c&ACK)==128);
}
int check_hdr_FLG(char c)
{
    return((c&FLG)==32);
}
int check_hdr_FIN(char c)
{
    return 0;
}
int check_hdr_END(char c)
{
    return ((c&END)==16);
}




/*
lab3-2部分:
*/
void make_pkg_num(unsigned char* buf,int pkg_num)
{
    /*
        将序列号填入buf相应位置
        目前序列号最大可取32位
    */
    buf[4]=pkg_num;
    buf[5]=pkg_num>>8;
    buf[6]=pkg_num>>16;
    buf[7]=pkg_num>>24;
}
int read_pkg_num(unsigned char* buf)
{
    /*
        读取buf中的序列号并返回
    */
    int num=0;
    int tmp[4];
    tmp[0]=buf[4];
    tmp[1]=buf[5];
    tmp[1]=tmp[1]<<8;
    tmp[2]=buf[6];
    tmp[2]=tmp[2]<<16;
    tmp[3]=buf[7];
    tmp[3]=tmp[3]<<24;
    num=tmp[0]+tmp[1]+tmp[2]+tmp[3];
    return num;
}
int lab3_2_Sendto(int fd, size_t n, int flags, const struct sockaddr *addr, socklen_t addr_len,int PRO,struct WindowItem* item,int pkg_num)
{
    /*
        集成make_hdr make_sum make_pkg_num sendto
    */
    make_hdr(item->send_buf,PRO);
    make_sum(item->send_buf,n);
    make_pkg_num(item->send_buf,pkg_num);
    item->pkg_num=pkg_num;
    item->pkg_size=n;
    return sendto(fd,item->send_buf,n,flags,addr,addr_len);
}

