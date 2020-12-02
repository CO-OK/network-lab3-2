#include"msghdr.h"
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

