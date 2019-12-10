#include "types.h"
#include "stat.h"
#include "defs.h"
#include "param.h"
#include "traps.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "file.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"

typedef int (*procfs_func)(char*);
//allow char and int as return value
int currentInodeNum = 0;



int
procfsisdir(struct inode *ip) {
    if (currentInodeNum == 0){
        struct superblock sb;
        readsb(ip->dev, &sb);
        currentInodeNum = sb.ninodes;
    }
    if (!(ip->type == T_DEV) || !(ip->major == PROCFS))
        return 0;
    int inum = ip->inum;
    if (inum == (currentInodeNum+1) || inum == (currentInodeNum+2))
        return 0;
    int val = (inum < currentInodeNum || inum % 10 == 0);
    return val;
}
int ideinfo(char *ansBuf){

    writeToBuffer(ansBuf, "Waiting operations: ");
    appendNumToBufEnd(ansBuf, getWaitingOperations());
    writeToBuffer(ansBuf, "\nRead waiting operations: ");
    appendNumToBufEnd(ansBuf, getReadWaitingOperations());
    writeToBuffer(ansBuf, "\nWrite waiting operations: ");
    appendNumToBufEnd(ansBuf, getWriteWaitingOperations());
    writeToBuffer(ansBuf, "\n");
    return strlen(ansBuf);
}
void
procfsiread(struct inode* dp, struct inode *ip) {

    ip->valid = 1;
    ip->major = PROCFS;
    ip->type = T_DEV;
    ip->nlink = 1;

}
//----------------------------
//functions for getting files:
#define GetMin(a, b) ((a) < (b) ? (a) : (b))

void writeToBuffer(char *buff, char *text) {
    int len = strlen(text);
    //get text's len
    int sz = strlen(buff);
    //then move data according to len
    memmove(buff + sz, text,len);

}

void writeDirToBuffer(char *dirName, char *buff, int dPlace, int inum) {

    struct dirent dirent1;
    dirent1.inum = inum;
    //assign the given inum
    memmove(&dirent1.name, dirName, strlen(dirName)+1);
    //move
    memmove(buff + dPlace*sizeof(dirent1) , (void*)&dirent1, sizeof(dirent1));
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

char* itoa( char* buffer,int n)
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


void appendNumToBufEnd(char *buff, int num){
    char numContainer[10] = {0};
    itoa(numContainer, num);
    writeToBuffer(buff, numContainer);
}

int fillProcDirents(char *buff1){
    int pids[NPROC] = {0};
    writeDirToBuffer(".", buff1, 0, namei("/proc")->inum);
    writeDirToBuffer("..", buff1, 1, namei("/")->inum);
    writeDirToBuffer("ideinfo", buff1, 2, currentInodeNum + 1);
    writeDirToBuffer("filestat", buff1, 3, currentInodeNum + 2);
    getUsedPIDs(pids);
    int i, k =4 ;
    char numContainer[3] = {0};
    for (i = 0; i<NPROC; i++){
        if (pids[i] != 0){
            numContainer[0] = 0;
            numContainer[1] = 0;
            numContainer[2] = 0;
            itoa(numContainer, pids[i]);
            writeDirToBuffer(numContainer, buff1, k, currentInodeNum + (i + 1) * 10);
            k++;
        }
    }
    writeDirToBuffer("inodeinfo", buff1, k, currentInodeNum + 660);
    k++;
    return sizeof(struct dirent)*k;
}

int GetProcessDir(char *ansBuf){
    short slot = ansBuf[0];
    int pid = getPID(slot);
    char dirPath[9] = {0};
    writeToBuffer(dirPath, "/proc/");
    appendNumToBufEnd(dirPath, pid);
    writeDirToBuffer(".", ansBuf, 0, namei(dirPath)->inum);
    writeDirToBuffer("..", ansBuf, 1, namei("/proc")->inum);
    writeDirToBuffer("name", ansBuf, 2, currentInodeNum + 1 + (slot + 1) * 10);
    writeDirToBuffer("status", ansBuf, 3, currentInodeNum + 2 + (slot + 1) * 10);
    return sizeof(struct dirent)*4;
}


int filestatus(char *ansBuf){
    writeToBuffer(ansBuf, "Free fds: ");
    appendNumToBufEnd(ansBuf, getFreeFdsCount());
    writeToBuffer(ansBuf, "\nUnique inodes fds: ");
    appendNumToBufEnd(ansBuf, getUniqueInodesCount());
    writeToBuffer(ansBuf, "\nWritable fds: ");
    appendNumToBufEnd(ansBuf, getWritableFdsCount());
    writeToBuffer(ansBuf, "\nReadable fds: ");
    appendNumToBufEnd(ansBuf, getReadableFdsCount());
    writeToBuffer(ansBuf, "\nRefs per fds: ");
    appendNumToBufEnd(ansBuf, getRefsPerFdsCount());
    writeToBuffer(ansBuf, "\n");
    return strlen(ansBuf);
}


int Getinodeinfo(char *ansBuf){

    writeDirToBuffer("..", ansBuf, 0, namei("/proc")->inum);
    writeDirToBuffer(".", ansBuf, 1, namei("/proc/inodeinfo")->inum);
    struct inode* inode1;
    int i;
    int k = 2;
    char numContainer[3] = {0};
    for (i = 0; i < NINODE; i++){
        inode1=getFileInIndex(i);
        if (inode1->type!=0){
            itoa(numContainer, i);
            writeDirToBuffer(numContainer, ansBuf, k, currentInodeNum + 660 + i + 1);
            k++;
        }
    }
    return sizeof(struct dirent)*k;
}



int inodestat(char *ansBuf){
    int freeInodeCount = getFreeInodeNum();
    writeToBuffer(ansBuf, "Free inodes: ");
    appendNumToBufEnd(ansBuf, freeInodeCount);
    writeToBuffer(ansBuf, "\nValid inodes: ");
    appendNumToBufEnd(ansBuf, getValidCount());
    writeToBuffer(ansBuf, "\nRefs per inode: ");
    appendNumToBufEnd(ansBuf, getRefNum());
    writeToBuffer(ansBuf, "\nRefs per inode: ");
    appendNumToBufEnd(ansBuf, getRefNum());
    writeToBuffer(ansBuf, "/");
    appendNumToBufEnd(ansBuf, NINODE-freeInodeCount);
    writeToBuffer(ansBuf, "\n");
    return strlen(ansBuf);
}

int inodeinfo(char *ansBuf){
    short slot = ansBuf[0];
    ansBuf[0] = 0;
    ansBuf[1] = 0;
    struct inode *inode1 = getFileInIndex(slot);
    if (inode1 == 0)
        return 0;

    writeToBuffer(ansBuf, "device: ");
    appendNumToBufEnd(ansBuf, inode1->dev);
    writeToBuffer(ansBuf, "\ntype: ");
    appendNumToBufEnd(ansBuf, inode1->type);
    writeToBuffer(ansBuf, "\ninum: ");
    appendNumToBufEnd(ansBuf, inode1->inum);
    writeToBuffer(ansBuf, "\nvalid: ");
    appendNumToBufEnd(ansBuf, inode1->valid);
    writeToBuffer(ansBuf, "\nmajor: ");
    appendNumToBufEnd(ansBuf, inode1->major);
    writeToBuffer(ansBuf, "\nminor: ");
    appendNumToBufEnd(ansBuf, inode1->minor);
    writeToBuffer(ansBuf, "\nhard links: ");
    appendNumToBufEnd(ansBuf, inode1->nlink);
    writeToBuffer(ansBuf, "\nsize/blocks used: ");
    appendNumToBufEnd(ansBuf, inode1->size);
    writeToBuffer(ansBuf, "\n");
    return strlen(ansBuf);
}




//------------------------------



procfs_func fileGetter(struct inode *ip){

    if (ip->inum < currentInodeNum)
        return &fillProcDirents;
    if (ip->inum == (currentInodeNum+1))
        return &ideinfo;
    if (ip->inum == (currentInodeNum+2))
        return &filestatus;
    if (ip->inum == (currentInodeNum+660))
        return &Getinodeinfo;
    if (ip->inum > (currentInodeNum+660))
        return &inodeinfo;
    if (ip->inum % 10 == 0)
        return &GetProcessDir;
    if (ip->inum % 10 == 1)
        return &getName;
    if (ip->inum % 10 == 2)
        return &status;
    cprintf("Got invalid inum\n");
    return 0;
}

int
procfsread(struct inode *ip, char *dst, int off, int n) {

    if (currentInodeNum == 0){
        struct superblock sb;
        readsb(ip->dev, &sb);
        currentInodeNum = sb.ninodes;
    }
    char ansBuf[1100] = {0}; //longest data is 66 dirents * 16 bytes
    int ansSize;
    procfs_func f = fileGetter(ip);
    short slot = 0;
    if (ip->inum >= currentInodeNum+660){
        ansBuf[0] = ip->inum -currentInodeNum - 660;
    }
    else if (ip->inum >= currentInodeNum+10){
        slot = (ip->inum-currentInodeNum)/10 - 1;
        ansBuf[0] = slot;
        short midInum = ip->inum % 10;
        if (midInum >= 10)
            ansBuf[1] = midInum-10;
    }

    ansSize = f(ansBuf);
    memmove(dst, ansBuf+off, n);
    return GetMin(n, ansSize-off);
    
}

int
procfswrite(struct inode *ip, char *buf, int n)
{
    return -1;
}

void
procfsinit(void)
{
    devsw[PROCFS].isdir = procfsisdir;
    devsw[PROCFS].write = procfswrite;
    devsw[PROCFS].iread = procfsiread;
    devsw[PROCFS].read = procfsread;
}