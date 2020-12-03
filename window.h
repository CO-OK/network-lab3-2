#include"msghdr.h"
#include<sys/socket.h>
#include<stdlib.h>
#include<stdio.h>
#define time_table_num 4
#define window_num 5
struct WindowItem{
    unsigned char recv_buf[buf_len];
    unsigned char send_buf[buf_len];
    struct WindowItem* next;
    int pkg_num;
    int pkg_size;
};

struct ServerWindow{
    int size;
    struct WindowItem* head;
    struct WindowItem* tail;
    struct WindowItem* next_seq_num;
};
int MakeServerWindow(struct ServerWindow*  window,int n);
int DeleteWindow(struct ServerWindow*  window);
struct thread_item{
    socklen_t* saddrlen;
    struct ServerWindow *window;
    struct sockaddr_in* saddr;
    char* file_name;
    int timer_stop;
    int count;
    int pos;
    int sock;
    int port;
    int shake_hand_done;
    //struct itimerval* time_set;
};

struct TimeTableItem
{
    int valid;
    struct thread_item* item;  
};

struct TimeTable
{
    struct TimeTableItem table[time_table_num];
};
int find_next_pos(struct TimeTable* table);
int TimeTableInsert(struct TimeTable* time_table,struct thread_item*item,int pos);

struct SockPort
{
    int port;
    int valid;
};

