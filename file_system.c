#include "lib_disk.h"

//---------------------------------------------------------------
//                  Local Function Definition                   |
//---------------------------------------------------------------
int _get_block(int fd, int blk, char *buf);
int _put_block(int fd, int blk, char *buf);

//---------------------------------------------------------------
// function:    fs_mount_root                                   |
// inputs:      none                                            |
// explanation: mounts the filesystem, establishes / and CWDs   |
//---------------------------------------------------------------
int fs_mount_root(void)
{
    int return_val;
    char device_path[SMALL_BUFFER_SIZE];
    char buf[LARGE_BUFFER_SIZE];

    printf("enter rootdev name(RETURN for /dev/fd0): ");
    scanf("%s", device_path);
    //open device for RW
    dev = open(device, O_RDWR);
    //check if device wasn't open
    if(dev < 0)
    {
        assert(0);
        return FAILURE;
    }

    //read SUPER block to verify it's an EXT2 FS
    if(_get_block(dev, SUPERBLOCK, buf) < 0)
    {
        return FAILURE;
    }

    //get pointer to super block
    g_sp = (SUPER *)buf;
    if(g_sp->s_magic != EXT2FS_MAGIC_NUM)
    {
        printf("NOT A EXT2FS!\n");
        return FAILURE;
    }

    // get root inode and store into global root
    g_root = iget(dev, GDBLOCK);

    //Let cwd of both P0 and P1 point at the root minode (refCount=3)
    P0.cwd = iget(dev, GDBLOCK);
    P1.cwd = iget(dev, GDBLOCK);

    return SUCCESS;
}

//---------------------------------------------------------------
// function:    _get_block                                      |
// inputs:      fd:     file discriptor                         |
//              blk:    block number on EXT2 FS Filesystem      |
//              buf:    buffer to read into                     |
// explanation: mounts the filesystem                           |
//---------------------------------------------------------------
int _get_block(int fd, int blk, char *buf)
{
    int return_val  = FAILURE;

    return_val = lseek(fd, (long)(blk*1024), 0);
    if(return_val >= SUCCESS)
    {
        return_val = read(fd, buf, 1024);
    }

    return return_val;
}

//---------------------------------------------------------------
// function:    _put_block                                      |
// inputs:      fd:     file discriptor                         |
//              blk:    block number on EXT2 FS Filesystem      |
//              buf:    buffer to read into                     |
// explanation: mounts the filesystem                           |
//---------------------------------------------------------------
int _put_block(int fd, int blk, char *buf)
{
    int return_val;  = FAILURE;

    return_val = lseek(fd, (long)(blk*1024), 0);
    if(return_val >= SUCCESS)
    {
        return_val = write(fd, buf, 1024);
    }

    return return_val;
}
