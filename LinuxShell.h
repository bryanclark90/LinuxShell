//	this program runs on a created linux fd image and simulates a simple linux shell

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

GD    *gp;
SUPER *sp;
INODE *ip;
DIR   *dp;

#define BLKSIZE 1024
#define BLOCK_SIZE        1024
#define BITS_PER_BLOCK    (8*BLOCK_SIZE)
#define INODES_PER_BLOCK  (BLOCK_SIZE/sizeof(INODE))

// Block number of EXT2 FS on FD
#define SUPERBLOCK        1
#define GDBLOCK           2
#define BBITMAP           3
#define IBITMAP           4
#define INODEBLOCK        5
#define ROOT_INODE        2

// Default dir and regulsr file modes
#define DIR_MODE          0040777
#define FILE_MODE         0100644
#define SUPER_MAGIC       0xEF53
#define SUPER_USER        0

// Proc status
#define FREE              0
#define READY             1
#define RUNNING           2

// Table sizes
#define NMINODES         100
#define NMOUNT            10
#define NPROC             10
#define NFD               10
#define NOFT             100

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
PROC* running;
PROC* readQueue;
MINODE minode[100];
MINODE* root;
char* device = "mydisk";
int dev;
int n = 0, y = 0, w = 0;

void init(void);
int get_block(int fd, int blk, char *buf);
int put_block(int fd, int blk, char *buf);
unsigned long getino(int* device1, char* pathname);
unsigned long search(MINODE* mip, char* name);
MINODE *iget(int dev,int ino);
MINODE *iput(MINODE *mip);
int mount_root(void);
void ls(char* pathname);
int cd(char *pathname);
char* permissionString(unsigned short mode);
char* lsInfo(INODE* ip, char* string);
int do_stat(char* pathname, struct stat* mystat);
void pwd(void);
int do_pwd(int ino);
int make_dir(char pathname[256]);
int my_mkdir(MINODE* pip, char* name);
int rm_dir(char* pathname);
int rm_child(MINODE* parent, char* my_name);
int do_touch(char *pathname);
int decFreeInodes(int dev);
int incFreeInodes(int dev);
int decFreeBlocks(int dev);
int incFreeBlocks(int dev);
int tst_bit(char *buf, int i);
int set_bit(char *buf, int i);
int clear_bit(char *buf, int i);
unsigned long ialloc(int dev);
unsigned long idalloc(int dev, int clear);
unsigned long balloc(int dev);
unsigned bdalloc(int dev, int clear);
int CommandToke(char part1[], char part2[]);
void PathToke(char pathname[], char part1[], char part2[], int length);
int linkToke(char part1[], char part2[]);
int unlink(char *pathname);
int my_symlink(char oldFileName[], char newFileName[]);
int readlink(char* pathname);
void menu(void);
int my_chmod(char newPermission[], char pathname[]);
int truncate(MINODE *mip);
int DTbdalloc(MINODE *mip);
int falloc(int check);
int fdalloc(OFT *oftptr);
int open_file(char pathname[], char mode[]);
int ModeConvert (char *mode);
int *FileCheck(MINODE *mip, int mode);
int close_file(int fd);
void pfd(void);
long read_file(int fd, int nbytes, int print);
long my_read(int fd, char *buf, int nbytes);
long write_file(int fd, char *text, int print);
long my_write(int fd, char *buf, int nbytes);
int cat_file(char *pathname);
long lseek_file(int fd, long newPosition);
