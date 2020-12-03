#include"window.h"
#include <stdlib.h>
int MakeServerWindow(struct ServerWindow* window,int n)
{
    
    window->size=n;
    struct WindowItem*tmp;
    tmp=window->head=malloc(sizeof(struct WindowItem));
    for(int i=0;i<n-1;i++)
    {
        tmp->next=(struct WindowItem*)malloc(sizeof(struct WindowItem));
        window->tail=tmp;
        tmp=tmp->next;
    }
    window->tail=tmp;
    //tmp=window->head;
    window->tail->next=window->head;
    window->next_seq_num=window->head; 
    return 0;
}
int DeleteWindow(struct ServerWindow*  window)
{
    struct WindowItem*tmp=window->head;
    struct WindowItem*tmp_next=window->head;
    for(int i=0;i<window->size;i++)
    {
        tmp=tmp_next;
        tmp_next=tmp->next;
        free(tmp);
    }
    return window->size;
}
int find_next_pos(struct TimeTable* table)
{
    for(int i=0;i<time_table_num;i++)
    {
        if(table->table[i].valid==1)
            return i;
    }
    return -1;
}
int TimeTableInsert(struct TimeTable* time_table,struct thread_item*item,int pos)
{
    time_table->table[pos].item=item;
    time_table->table[pos].item->timer_stop=1;
    time_table->table[pos].item->count=0;
    time_table->table[pos].valid=0;
    item->pos=pos;
    return 0;
}