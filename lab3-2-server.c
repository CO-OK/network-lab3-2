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
void shake_hand(struct sockaddr* saddr,socklen_t*saddrlen,int sock,struct thread_item* item);
void say_goodbye();
int make_sum( unsigned char * buf,int len);
int check_sum(unsigned char* buf,int len);
int Sendto(int fd,  void *buf, size_t n, int flags, const struct sockaddr *addr, socklen_t addr_len,int PRO);
void ALRM_handler();
int set_timer(int n_msecs);
int CheckArg(int ac,char*av[]);


struct itimerval time_set;
struct TimeTable time_table;
void  thread_send(void *arg);
void  thread_recv(void *arg);
int lab3_2_Sendto(int fd, size_t n, int flags, const struct sockaddr *addr, socklen_t addr_len,int PRO,struct WindowItem* item,int pkg_num);
int read_pkg_num(unsigned char* buf);
int make_thread_item(struct thread_item* item);
int find_next_port(int len);
struct SockPort* port_list;
void delay(unsigned i);
int main(int ac,char*av[])
{
    
    signal(SIGALRM,ALRM_handler);//为计时器设置好处理函数
    //检测参数是否合法1..ac-1
    CheckArg(ac,av);
    //创建服务器套接字
    
    port_list=malloc(sizeof(struct SockPort)*(ac-2));

    for(int i=0;i<(ac-2);i++)
    {
        port_list[i].port=atoi(av[i+1]);
        port_list[i].valid=1;
    }
    //初始化time_table
    for(int i=0;i<time_table_num;i++)
        time_table.table[i].valid=1;
    //设置线程属性
    //port=atoi(av[1]);
    //设置计时器
    set_timer(1);
    while (1)
    {
        int port;
        if(find_next_pos(&time_table)!=-1&&(port=find_next_port(time_table_num))!=-1)
        {
            int sock;
            printf("port: %d\n",port);
            if((sock=make_dgram_server_socket(port,1))==-1)
                oops("can not make socket",2);
            struct thread_item* item=malloc(sizeof(struct thread_item));//创建一个新的线程结构体，方便传参
            make_thread_item(item);//初始化该结构体
            item->window=malloc(sizeof(struct ServerWindow));//初始化窗口
            item->file_name=av[ac-1];//要打开的文件名
            item->sock=sock;
            item->port=port;
            item->shake_hand_done=-1;
            pthread_t thread_send_t,thread_recv_t;
            if((pthread_create(&thread_send_t,NULL,(void*)&thread_send,(void*)(item)))!=0)
            {
                oops("there is somthing wrong when create a new thread\n",1);
            }
            if((pthread_create(&thread_recv_t,NULL,(void*)&thread_recv,(void*)(item)))!=0)
            {
                oops("there is somthing wrong when create a new thread\n",1);
            }
            printf("done!\n");
        }  
    }
    return 0;
}




void shake_hand(struct sockaddr* saddr,socklen_t*saddrlen,int sock,struct thread_item* item)
{
     //服务器端的握手
    unsigned char buf_send[buf_len];
    unsigned char buf_recv[buf_len];
    while(1)
    {
        recvfrom(sock,buf_recv,buf_len,0,saddr,&(*saddrlen));
        if(check_hdr_SYN(buf_recv[0]))
        {
            Sendto(sock,buf_send, buf_len, 0, saddr,*saddrlen,SYN|ACK);
            //设置计时器
            item->shake_hand_done=0;
            break;
        }
    }
    while(1)
    {
        recvfrom(sock,buf_recv,buf_len,0,saddr,&(*saddrlen));
        if(check_hdr_ACK(buf_recv[0]))
        //计时器失效
        {
            item->shake_hand_done=1;
            break;
        }
            
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
    return setitimer(ITIMER_REAL,&tmp_time_set,&time_set);
}

void ALRM_handler()
{
    for(int i=0;i<time_table_num;i++)
    {
        if(time_table.table[i].valid==0)
        {
            time_table.table[i].item->count++;//对所有表项进行计数
            if(time_table.table[i].item->count==1000&&time_table.table[i].item->shake_hand_done==1)//对到时的进行重发
            {
                time_table.table[i].item->count=0;//重新计时
                if(!time_table.table[i].item->timer_stop)
                {
                    printf("resend port %d\n",time_table.table[i].item->port);
                    struct WindowItem*tmp=time_table.table[i].item->window->head;
                    while(tmp!=time_table.table[i].item->window->next_seq_num)
                    {
                        if(time_table.table[i].valid==0)
                        {
                            printf("resend %d\n",read_pkg_num(tmp->send_buf));
                            lab3_2_Sendto(time_table.table[i].item->sock, tmp->pkg_size, 0, (struct sockaddr*)time_table.table[i].item->saddr,\
                                            *time_table.table[i].item->saddrlen,tmp->send_buf[0],tmp,tmp->pkg_num);
                            usleep(2000);
                            tmp=tmp->next;
                        }
                        else
                        {
                            break;
                        }
                       
                    }
                }
            }
            //若握手时出现丢包
            else if(time_table.table[i].item->shake_hand_done==0&&time_table.table[i].item->count==3000)
            {
    
                printf("shake_hand_done:%d\n",(time_table.table[i].item->shake_hand_done));
                char buf_send[buf_len];
                time_table.table[i].item->count=0;//重新计时
                //重发
                Sendto(time_table.table[i].item->sock,buf_send, buf_len, 0, (struct sockaddr*)time_table.table[i].item->saddr,*time_table.table[i].item->saddrlen,SYN|ACK);
            }
        }
    }
}


void thread_send(void *arg)
{
    
    int pkg_num=0;
    int nchars=0;
    struct thread_item* item=arg;
    
    int pos=find_next_pos(&time_table); //找到时间表中一个合适的位置  
    printf("pos:%d\n",pos);  
    TimeTableInsert(&time_table,item,find_next_pos(&time_table));//将项目加入表项
    MakeServerWindow(item->window, window_num);
    shake_hand((struct sockaddr*)item->saddr,item->saddrlen,item->sock,item);//握手
    //item->shake_hand_done=1;
    item->timer_stop=0;
    int fd=open(item->file_name,O_RDONLY); 
    printf("open\n");  
    while(1)
    {
        if(item->window->tail!=item->window->next_seq_num)
        {
            nchars=read(fd,item->window->next_seq_num->send_buf+offset,buf_len-offset);
            if(nchars==0)
            {
                lab3_2_Sendto(item->sock, nchars+offset, 0, (struct sockaddr*)item->saddr,*item->saddrlen,END,item->window->next_seq_num,pkg_num);
                printf("port %d send end pkg %d\n",item->port,pkg_num);
                close(fd); 
                item->count=0;
                item->window->next_seq_num=item->window->next_seq_num->next;
                printf("port %d server send done\n",item->port);
                break;
            }
            lab3_2_Sendto(item->sock, nchars+offset, 0, (struct sockaddr*)item->saddr,*item->saddrlen,ACK,item->window->next_seq_num,pkg_num);
            usleep(2000);
            printf("port %d send pkg %d , len=%d\n",item->port,pkg_num,offset+nchars);
            pkg_num++;
            if(item->window->head==item->window->next_seq_num)
            {
                item->count=0;
            }      
            item->window->next_seq_num=item->window->next_seq_num->next;
        }
    }
}

void thread_recv(void *arg)
{
    struct thread_item* item=arg;
    int nchars=0;
    unsigned char recv[buf_len];
    while(item->shake_hand_done!=1);
    while(1)
    {
        nchars=recvfrom(item->sock,recv,buf_len,0,(struct sockaddr*)item->saddr,&(*item->saddrlen));
        if(check_sum(recv,nchars)&&read_pkg_num(recv)==item->window->head->pkg_num)
        {
            printf("port=%d recv pkg %d\n",item->port,read_pkg_num(recv));
            item->window->head=item->window->head->next;
            item->window->tail=item->window->tail->next;
            if(item->window->head==item->window->next_seq_num)
                item->timer_stop=1;
            if(check_hdr_END(recv[0]))
            {
                break;
            }
        }
    }
    //清除表项中相应位置
    time_table.table[item->pos].valid=1;
    //关闭文件描述符
    close(time_table.table[item->pos].item->sock);
    //释放端口
    for(int i=0;i<time_table_num;i++)
        if(port_list[i].port==item->port)
            port_list[i].valid=1;
    //释放线程资源
    //窗口shifang
    printf("delete window num=%d\n",DeleteWindow(item->window));
    free(item->window);
    printf("thread window deleted\n");
    //删除线程参数结构体
    free(item);
    printf("one thread finished\n");
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
int make_thread_item(struct thread_item* item)
{
    item->saddr=malloc(sizeof(struct sockaddr_in));
    item->saddrlen=malloc(sizeof( socklen_t));
    *item->saddrlen=sizeof(*item->saddr);
    //item->window=
    return 0;
}

int CheckArg(int ac,char*av[])
{
    if(ac!=2+time_table_num)
        oops("args num error\n",1);
    /*for(int i=1;i<=ac-1;i++)
    {
        for(int j=0;j<strlen(av[i]);j++)
            if(av[i][j]<'0'||av[i][j]>'9')
                oops("wrong port", 2);
    }*/
    return 0;
}

int find_next_port(int len)
{
    for(int i=0;i<len;i++)
    {
        if(port_list[i].valid==1)
        {
            printf("index=%d\n",i);
            port_list[i].valid=0;
            return port_list[i].port;
        }
    }
    return -1;
}