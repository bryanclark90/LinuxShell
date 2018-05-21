#include "file_system.h"

//---------------------------------------------------------------
//                  Globals Definition                          |
//---------------------------------------------------------------
GD*     g_gp;                   //global group descriptor pointer
SUPER*  g_sp;                   //global super group pointer
INODE*  g_ip;                   //global inode pointer
DIR*    g_dp;                   //global directory pointer
PROC    P0;                     //Process 0 (running pointer)
PROC    P1;                     //Process 1 (readQueue pointer)
PROC*   g_running;              //Process 0
PROC*   g_readQueue;            //Process 1
MINODE  g_minode[NUM_MINODES];  //global inodes loaded into memory
MINODE* g_root;                 //global pointer to root inode in memory
int     dev;                    //the disk we're accessing
int     n, y, w;                //?????

//---------------------------------------------------------------
//                  Local Function Definition                   |
//---------------------------------------------------------------
int     _get_block  (int fd, int blk, char *buf);
int     _put_block  (int fd, int blk, char *buf);
MINODE* _iget       (int dev,int ino);

//---------------------------------------------------------------
// function:    fs_mount_root                                   |
// inputs:      none                                            |
// explanation: mounts the filesystem, establishes / and CWDs   |
//---------------------------------------------------------------
int fs_mount_root(void)
{
    char device_path[SMALL_BUFFER_SIZE];
    char buf[LARGE_BUFFER_SIZE];

    printf("enter rootdev name(RETURN for /dev/fd0): ");
    scanf("%s", device_path);
    //open device for RW
    dev = open(device_path, O_RDWR);
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
    g_root = _iget(dev, GDBLOCK);

    //Let cwd of both P0 and P1 point at the root minode (inodes in memory)(refCount=3)
    P0.cwd = _iget(dev, GDBLOCK);
    P1.cwd = _iget(dev, GDBLOCK);

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

    return_val = lseek(fd, (long)(blk*BLOCK_SIZE), 0);
    if(return_val >= SUCCESS)
    {
        return_val = read(fd, buf, BLOCK_SIZE);
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
    int return_val  = FAILURE;

    return_val = lseek(fd, (long)(blk*BLOCK_SIZE), 0);
    if(return_val >= SUCCESS)
    {
        return_val = write(fd, buf, BLOCK_SIZE);
    }

    return return_val;
}

//---------------------------------------------------------------
// function:    _iget                                           |
// inputs:      dev:    file discriptor                         |
//              ino:    The inode block we want to access.      |
//                      inodes contain information disk block   |
//                      locations, and meta data like access    |
//                      date and last modification              |
// explanation: loads an INDOE into memory and returns a ptr    |
//---------------------------------------------------------------
MINODE* _iget(int dev,int ino)
{
   int i;
   GD* group_descriptor_p;
   //initalize MINODE pointer to root loaded in main.
   MINODE* mip          = g_root;
   INODE *ip            = NULL;
   int iNodeBeginBlock  = 0;
   int offset           = 0;
   int blk              = INVALID_BLOCK;
   char buf[BLOCK_SIZE];

   //search minode[ ] array for an item pointed by mip with the SAME (dev,ino)
   for (i=0; i < NUM_MINODES; i++)
   {
       mip = &g_minode[i];
       if ((mip->refCount) && (mip->dev== dev) && (mip->ino == ino))
       {
           mip->refCount++;
           return mip;
       }
   }
   // needed INODE not in memory
    //search minode[ ] array for a mip whose refCount=0:
    for (i=0; i < NUM_MINODES; i++)
    {
        if (g_minode[i].refCount != 0)
        {
            mip = &g_minode[i];
            break;
        }
    }
    // set refCount to 1
    // set (dev, ino)
    //Init dirty, mounted, and pointer to mount.
    mip->refCount   = 1;
    mip->dev        = dev;
    mip->ino        = ino;
    mip->dirty      = 0;
    mip->mounted    = 0;
    mip->mountPtr   = 0;

   //Use Mailman's algorithm to convert ino to
   //blk    = disk block containing the needed INODE
   if(_get_block(dev, SUPERBLOCK, buf) < 0)
   {
       mip = g_root;
   }
   else
   {
       group_descriptor_p = (GD *) buf;
       iNodeBeginBlock = group_descriptor_p->bg_inode_table;
       //find the correct blk
       blk = GET_INODE_BLOCK(ino, iNodeBeginBlock);
       //offset = INODE offset in that blk
       offset = GET_INODE_OFFSET(ino);
       if(_get_block(dev, blk, buf) < 0)
       {
           mip = g_root;
       }
       else
       {
           // point memory inode at INODE, which is the
           // the block buffer pointer at the offest
           ip = (INODE *) buf + offset;
           mip->inode = *ip;
       }
    }

   return mip;

}
