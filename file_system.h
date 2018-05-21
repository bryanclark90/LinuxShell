#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <sys/stat.h>
#include <ext2fs/ext2_fs.h>
#include <libgen.h>
#include <time.h>

//------------------------------------------------
//        Magic Number/Macros Definition         |
//------------------------------------------------
#define FAILURE    -1
#define SUCCESS     0

//block size information
#define BITS_IN_BYTE            8
#define BLOCK_SIZE              1024
#define BITS_PER_BLOCK          (BITS_IN_BYTE*BLOCK_SIZE)
#define INODES_PER_BLOCK        (BLOCK_SIZE/sizeof(INODE))
#define GET_INODE_BLOCK(x, y)   (((x-1)/BITS_IN_BYTE) + y)
#define GET_INODE_OFFSET(x)     ((x-1)%BITS_IN_BYTE)
//char buffer size
#define SMALL_BUFFER_SIZE   256
#define LARGE_BUFFER_SIZE   1024

// Block number of EXT2 FS on FD
#define INVALID_BLOCK      -1
#define SUPERBLOCK          1
#define GDBLOCK             2
#define BBITMAP             3
#define IBITMAP             4
#define INODEBLOCK          5
#define ROOT_INODE          2

// Default dir and regular file modes
#define DIR_MODE            0040777
#define FILE_MODE           0100644
#define EXT2FS_MAGIC_NUM    0xEF53
#define SUPER_USER          0

// Proc status
#define FREE                0
#define READY               1
#define RUNNING             2

// Table sizes
#define NUM_MINODES         100
#define NMOUNT              10
#define NPROC               10
#define NFD                 10
#define NOFT                100

//------------------------------------------------
//             Structure Definitions              |
//------------------------------------------------
// define shorter TYPES, save typing efforts
typedef struct ext2_group_desc  GD;
typedef struct ext2_super_block SUPER;
typedef struct ext2_inode       INODE;
typedef struct ext2_dir_entry_2 DIR;    // need this for new version of e2fs

// Open File Table
typedef struct oft{
  int    mode;
  int    refCount;
  struct minode *inodeptr;
  int    offset;
  char   name[256];
}OFT;

// PROC structure
typedef struct proc{
  int   uid;
  int   pid;
  int   gid;
  int   ppid;
  struct proc *parent;
  int   status;
  struct minode *cwd;
  OFT   *fd[NFD];
}PROC;

// In-memory inodes structure
typedef struct minode
{
  INODE inode;               // disk inode
  int   dev, ino;
  int   refCount;
  int   dirty;
  int   mounted;
  struct mount *mountPtr;
}MINODE;

// Mount Table structure
typedef struct mount{
        int    ninodes;
        int    nblocks;
        int    dev, busy;
        MINODE *mounted_inode;
        char   name[256];
        char   mount_name[64];
}MOUNT;


//------------------------------------------------
//              Globals Definition               |
//------------------------------------------------
extern GD    *g_gp;
extern SUPER *g_sp;
extern INODE *g_ip;
extern DIR   *g_dp;

extern PROC P0;
extern PROC P1;
extern PROC* g_running;
extern PROC* g_readQueue;
extern MINODE g_minode[NUM_MINODES];
extern MINODE* g_root;
//char* device = "mydisk";
extern int dev;
extern int n, y, w;

//------------------------------------------------
//              Function Definition              |
//------------------------------------------------
int fs_mount_root(void);
