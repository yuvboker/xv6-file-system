#include "param.h"
#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "fcntl.h"
#include "syscall.h"
#include "traps.h"
//#include "defs.h"

#include "memlayout.h"


void append(char *buff, char * text){
    int textlen = strlen(text);
    int sz = strlen(buff);
    memmove(buff + sz, text,textlen);
}
inline void swap(char *x, char *y) {
    char t = *x; *x = *y; *y = t;
}
char* reverse(char *buffer, int i, int j)
{
    while (i < j)
        swap(&buffer[i++], &buffer[j--]);

    return buffer;
}
char* itoa(int n, char* buffer)
{
    int i = 0;
    while (n)
    {
        int r = n % 10;
        buffer[i++] = 48 + r;
        n = n / 10;
    }
    // if number is 0
    if (i == 0)
        buffer[i++] = '0';

    buffer[i] = '\0'; // null terminated

    return reverse(buffer, 0, i - 1);
}

int
main(int argc, char *argv[])
{
    printf(1, "lsnd starting\n");
    char buf[128] ;
    int fd;
    char *path=malloc(16);
    memmove(path,"proc/inodeinfo/",16);
    char *curPath = malloc(sizeof(char)*50);
    char *num="";

    for (int i=1;i<50;i++)
    {

        memmove(curPath,path,16);
        append(curPath,itoa(i,num));
        if((fd = open(curPath, 0)) < 0){
            //printf(2, "lsof: cannot open %s\n", curPath);
            continue;
        }
        read(fd, &buf, 128);
        printf(1,"\n%s\n",buf);
        close(fd);
    }
    exit();
}
