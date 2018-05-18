#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <ext2fs/ext2_fs.h>
#include <libgen.h>
#include <time.h>

// define shorter TYPES, save typing efforts
typedef struct ext2_group_desc  GD;
typedef struct ext2_super_block SUPER;
typedef struct ext2_inode       INODE;
typedef struct ext2_dir_entry_2 DIR;    // need this for new version of e2fs

GD    *g_gp;
SUPER *g_sp;
INODE *g_ip;
DIR   *g_dp;

#define FAILURE    -1
#define SUCCESS     0

//block size information
#define BLKSIZE             1024
#define BLOCK_SIZE          1024
#define BITS_PER_BLOCK      (8*BLOCK_SIZE)
#define INODES_PER_BLOCK    (BLOCK_SIZE/sizeof(INODE))

//char buffer size
#define SMALL_BUFFER_SIZE   256
#define LARGE_BUFFER_SIZE   1024

// Block number of EXT2 FS on FD
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
#define NMINODES            100
#define NMOUNT              10
#define NPROC               10
#define NFD                 10
#define NOFT                100

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


//Globals
PROC P0;
PROC P1;
PROC* g_running;
PROC* g_readQueue;
MINODE g_minode[100];
MINODE* g_root;
char* device = "mydisk";
int dev;
int n = 0, y = 0, w = 0;

//------------------------------------------------
//              Function Definition              |
//------------------------------------------------
int fs_mount_root(void);
