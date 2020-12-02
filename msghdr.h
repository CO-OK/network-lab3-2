#include<stdio.h>
#define ACK 0b10000000   //128
#define SYN 0b01000000   //64
#define FLG 0b00100000   //32
#define END 0b00010000   //16
#define FIN 0b00000001   //1

#define CLR 0b00000000

#ifndef offset
#define offset 8
#endif

#ifndef data_len
#define data_len BUFSIZ
#endif
#ifndef oops
#define oops(m,x)   {perror(m)  ;exit(x)    ;}
#endif
#ifndef buf_len
#define buf_len data_len+offset
#endif


