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