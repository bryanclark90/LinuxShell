//Bryan Clark 
//bryan.clark@email.wsu.edu
//509-294-4161


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


int main (int argc, char *argv[])
{
  char* cmd;
  char* toker;
  char* currentArg[1024];
  char* pathname1[1024];
  char* name1[1024];
  char* name2[1024];
  char tokeMe[256];
  char path[256];
  char path1[256];
  char path2[256];
  char parameter[256];
  int position = 0;
  char* pathname = 0;
  int i = 0;
  char comp[BLKSIZE];
  struct stat mystat;
  init();
  int allowed;
  int x;
  allowed = mount_root();
  //bomb out if not EXT2FS
  if(allowed == 1)
  {
    exit(1);
  }

getchar();
  while(1)
  {
  i = 0;
  char command[256];
  //ask for command string:
  printf("-------------------------------------------\n");
 
  n = CommandToke(command, path);
  if(strcmp(path, "$") == 0)
  {
     printf("Execute Command: %s\n", command);
  }
  else
  {
    printf("Execute Command: %s %s\n", command, path);
  }

 if(strcmp(command, "ls") == 0)
 {  
   ls(path);
 } 
 else if(strcmp(command, "cd") == 0)
 {
   cd(path);
 }
 else if(strcmp(command, "stat") == 0)
 {
   do_stat(path, &mystat);
 }
 else if(strcmp(command, "pwd") == 0)
 {
   pwd();
 }
 else if(strcmp(command, "mkdir") == 0)
 {
   make_dir(path);
 }
 else if(strcmp(command, "creat") == 0)
 {
   creat_file(path);
 }
 /*else if(strcmp(command, "rmdir") == 0)
 {
   rm_dir(path);
 }
 else if(strcmp(command, "unlink") == 0)
 {
   unlink(path);
 }*/
 else if(strcmp(command, "link") == 0)
 {
	y = linkToke(path1, path2);
	printf("Link %s %s\n", path1, path2);
	link(path1, path2);
 }
 else if(strcmp(command, "symlink") == 0)
 {
	y = linkToke(path1, path2);
	printf("Symlink %s %s\n", path1, path2);
	my_symlink(path1, path2);
 }
 else if(strcmp(command, "touch") == 0)
 {
   do_touch(path);
 }
 else if(strcmp(command, "menu") == 0)
 {
	menu();
 }
 else if(strcmp(command, "chmod") == 0)
 {
	y = chToke(path1, path2);
	printf("chmod %s %s\n", path1, path2);
	y = my_chmod(path1, path2);
 }
 else if(strcmp(command, "open") == 0)
 {
	y = chToke(path1, path2);
	printf("open %s for %s\n", path1, path2);
	y = open_file(path1, path2);
 }
 /*else if(strcmp(command, "close") == 0)
 {
	i = atoi(path);
	close_file(i);
 }*/
 else if(strcmp(command, "read") == 0)
 {
	printf("How many bytes would you like to READ? ");
	scanf("%s", parameter);
	i = atoi(path);
	position = atoi(parameter);
	read_file(i, position, 0);   
 }
 /*else if(strcmp(command, "write") == 0)
 {
	printf("What would you like to WRITE to file? ");
	scanf("%s", parameter);
	i = atoi(path);
	write_file(i, parameter, 1);
 }*/
 else if(strcmp(command, "cat") == 0)
 {
	cat_file(path);
 }
 else if(strcmp(command, "lseek") == 0)
 {
	printf("How many bytes would you like to offset? ");
	scanf("%s", parameter);
	i = atoi(path);
	position = atoi(parameter); 
	lseek_file(i, position);  
 }
 else if(strcmp(command, "pfd") == 0)
 {
   pfd();
 }
 else if(strcmp(command, "quit") == 0)
 {
   printf("-------------------------------------------\n");
   printf("Smell You Later, Nerd!\n");
   printf("-------------------------------------------\n");
    exit(1);
    
 }
 else
 {
	printf("%s Is Invalid: Choose from List Below:\n", command);
	printf("-------------------------------------------\n");
	menu();
 }
  }
  return 0;
  
}

int get_block(int fd, int blk, char *buf)
{
    lseek(fd, (long)(blk*1024), 0);
    read(fd, buf, 1024);
}
int put_block(int fd, int blk, char *buf)
{
    lseek(fd, (long)(blk*1024), 0);
    write(fd, buf, 1024);
}
//initializes data structs of Level 1
void init(void)
{
    // (1). 2 PROCs, P0 with uid=0, P1 with uid=1, all PROC.cwd = 0
      P0.uid = 0;
      P1.uid = 1;
      P0.cwd = 0;
      P1.cwd = 0;
  
    // (2). PROC *running -> P0;
      running = &P0;
    // (3). PROC *readQueue -> P1;
      readQueue = &P1;
      // (4). MINODE minode[100]; all with refCount=0
      int i;
      for(i = 0; i < 100; i++)
      {
	minode[i].refCount = 0;
      }
    // (5). MINODE *root = 0;
       root = 0;
}
unsigned long getino(int* dev, char* pathname)
{
  int ino = 0;
  char* toker;
  int i = 0;
  int j = 0;
  char buf[1024];
  int blk, offset;
  char* currentArg[1024];
  char* name1[1024];
  MINODE* mp = running->cwd;
  MINODE* mip;
  //if pathname starts at root

  if(pathname[0] == '/')
  {
    
    dev = root->dev;
    ino = root->ino;
  
  }
  else
  {
    dev = mp->dev;
    ino = mp->ino;
  }

  //tokenize the pathname
  toker = strtok(pathname, "/");
  while(toker != NULL)
  {
    
      currentArg[j] = malloc(sizeof(strlen(toker) + 1));
      name1[j] = (strcpy(currentArg[j], toker));
      toker = strtok(NULL, "/");
      j++;
  }
 
  //search all paths
  for(i=0;i<j;i++)
  {
    mip = iget(dev,ino);
    ino =search(mip,name1[i]);
    //if path doesn't exist
    if(ino == 0)
    {
      printf("Pathname %s does not exist\n", name1[i]);
      return 0;
    }
   char buf2[1024]; 
   get_block(mip->dev, 2, buf2);
   gp = (GD *)buf2;
   int inodeBeginBlock = gp->bg_inode_table;
   blk = (ino-1)/8+inodeBeginBlock;
   offset = (ino-1)%8;
   
   get_block(mip->dev, blk, buf);
   ip = (INODE*)buf+offset;
   if((i != j-1)&& !S_ISDIR(ip->i_mode))
   {
     printf("%s is not a directory!\n",name1[i]);
     return 0;
   }
    
  }
  
  return ino;
}
unsigned long search(MINODE* mip, char* name)
{
  //buffer to store dir into
  int i;
  char temp[256];
  char buf[1024];
  DIR* dp;
  char* cp;
int tempor;
  //get the block at mip
  for(i=0;i<12;i++)
  {
    if(mip->inode.i_block[i] == 0)
    {
      break;
    }
    get_block(dev, (mip->inode.i_block[0]), buf);  
    dp = (DIR*)(buf);
    cp = (buf);
  //check all dirs
    while(cp < (buf+BLKSIZE))
    {
      //copy DIR into temp buffer
      strncpy(temp, dp->name, dp->name_len);
      temp[dp->name_len] = 0;
      //if name at location pass in and temp equal
      tempor = strcmp(name, temp);
      if(tempor == 0)
      {
	return dp->inode;
      }
      cp += dp->rec_len;
      dp = (DIR*) cp;
     }
    return 0;
  }
}
MINODE *iget(int dev,int ino)  // return a mip -> minode containing INODE
{
   int i; 
   MINODE* mip = root;
   char buf[1024];
   //search for if exist
   for (i=0; i < NMINODES; i++)
   {
       mip = &minode[i];
       if (mip->refCount)
       {
          if ((mip->dev== dev) && (mip->ino == ino))
	  {
             mip->refCount++;
             return mip;
          }
       }
   }

   // needed INODE not in memory
   //Use Mailman's algorithm to convert ino to 
   //blk    = disk block containing the needed INODE
   char buf2[256];
   get_block(dev, 2, buf2);
   gp = (GD *)buf2;
   int inodeBeginBlock = gp->bg_inode_table;
   int blk;
   blk = (ino-1)/8 + inodeBeginBlock;
   //offset = INODE offset in that blk
   
    int offset;
    offset = (ino-1)%8;
    get_block(dev, blk, buf);
    INODE *ip = (INODE *)buf + offset;
    //find free minode
    int j = 0;
    
    while(minode[j].refCount != 0)
    {
      j++;
    }
    //mip = & of a FREE minode[];
    mip = &minode[j];
    // copy INODE: 
    // memcpy(&mip->inode, ip, sizeof(ip));
    mip->inode = *ip;

    // set refCount to 1   
    mip->refCount = 1;  
    mip->dev = dev;
    // set (dev, ino)
    mip->ino = ino; 
    mip->dirty = 0;              
    mip->mounted = 0; 
    mip->mountPtr = 0;
   
   return mip;
   
}
MINODE* iput(MINODE *mip)
{	
	int i, j;
	char buf[sizeof(INODE)]; 
	INODE *tmp = malloc(sizeof(INODE));
	mip->refCount--;
	if ( mip->refCount > 0 || mip->dirty == 0)
		return;
	if (mip->ino > 0)
	{	
	  lseek(dev, (long)((INODEBLOCK + ((mip->ino)-1)/INODES_PER_BLOCK) * BLOCK_SIZE + ((mip->ino)-1)%INODES_PER_BLOCK *sizeof(INODE)), 0);		
	  read(dev, buf, sizeof(INODE));
	  tmp = (INODE *)buf;
	  *(tmp) = mip->inode;
	  lseek(dev, (long)((INODEBLOCK + ((mip->ino)-1)/INODES_PER_BLOCK) * BLOCK_SIZE + ((mip->ino)-1)%INODES_PER_BLOCK *sizeof(INODE)), 0);
	  i = write(dev, tmp, sizeof(INODE));
	}
}
int mount_root(void)  // mount root file system, establish / and CWDs
{
   
      char device[256];
      char buf[1024];
      //opend device for RW
      //fd = open(dev, O_RDONLY||O_WRONLY);
      printf("enter rootdev name(RETURN for /dev/fd0): ");
      scanf("%s", device);
      dev = open(device, O_RDWR);
      //read SUPER block to verify it's an EXT2 FS
      get_block(dev, 1, buf);
      sp = (SUPER *)buf;
      if((sp->s_magic != 0xEF53))
      {
	printf("NOT A EXT2FS!\n");
	return 1;
      }
      /* get root inode */
      root = iget(dev, 2);    
      
      //Let cwd of both P0 and P1 point at the root minode (refCount=3)
      P0.cwd = iget(dev, 2); 
      P1.cwd = iget(dev, 2);
      return 0;
}
//List Sub Dir
void ls(char* pathname)
{
 
      int ino;
      int i;
      char buf[1024];
      char buf2[1024];
      char* cp;
      char lsString[BLKSIZE] = {0};
      char fileType;
      char permissions[BLKSIZE] = {0};
      char* timeString;//[BLKSIZE] = {0};
      char linkName[BLKSIZE] = {0};
      struct stat mystat;
      char *kpath=pathname;
      char temp[256];
      char pn[256]; //pathname to be passedc into string toke
      //current working dirrrrr
      MINODE* mp = running->cwd;
      MINODE* mip;
      ///???
     if((pathname)&&(strcmp(pathname, "$") != 0))
     {

        ino = getino(&mp->dev,pathname);
	strcat(pn, kpath);//put dirpath into pn
 
     }
     else //!!!!!!!!!!!!!!!need to put name of cwd into "pn"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
     {
	ino = mp->ino;

     }

     mip = iget(dev, ino); 
      // mip points at minode; 
      // Each data block of mip->INODE contains DIR entries
      
        char slash[55] = {0};
        sprintf(slash, "/");
	int namecount=0;
	char* lsOut;


     for(i = 0; i<12; i++)
     {
       
       if(mip->inode.i_block[i] == 0)
       {
	 break;
       }
	get_block(dev, mip->inode.i_block[i], buf);
	
	dp = (DIR*)(buf);
	cp = (buf);


	while(cp < (buf+BLKSIZE))
	{
	
	  strncpy(temp, dp->name, dp->name_len);
	  temp[dp->name_len] = 0;
	  //lsOut = lsInfo(&mip->inode, temp);
	  //printf("%s \n", lsOut);
	strcat(pn,slash);
	strcat(pn,temp);

//printf("%s\n", pn);

          do_stat(pn, &mystat);

//need to change how to find type

	  //if LINK
	  if((mystat.st_mode & 0120000) == 0120000)
	  {
	    fileType = 'l';
	  }
	  else if((mystat.st_mode & 0040000) == 0040000)
	  {
	    fileType = 'd';
	  }
	  else if((mystat.st_mode & 0100000) == 0100000)
	  {
	    fileType = '-';
	  }
	  
          timeString=ctime(&mystat.st_ctime);
	  timeString[strlen(timeString)-1] = 0;
	  //printf("here\n");

	  sprintf(permissions, "%s", permissionString(mystat.st_mode));
	  sprintf(lsString,"%c%s %3d %4d %4d %7d %20s %s", fileType, permissions, mystat.st_nlink, mystat.st_uid, mystat.st_gid, mystat.st_size, timeString, temp);
	  //printf("here\n");
	  if(fileType == 'l')
	  {
	    strcat(lsString,"->");
	    strcat(lsString,linkName);
	  }

	printf("%s \n", lsString);
	  //return permission string
	  //return lsString;


	  cp += dp->rec_len;
	  dp = (DIR*) cp;
	
	  strcpy(pn,kpath);//reset pn to just base
	  
	}
	// print the name strings of the DIR entries
     }
      iput(mip);
}
//Change Directory
int cd(char *pathname)
{
     int ino;
     //current CWD
     MINODE* mp = running->cwd;
     MINODE* mip;
     char* temp;
     
     if(pathname == 0)
     {
       iput(mp);
       running->cwd = root;
       root->refCount++;
       return 1;
     }
     //get ino
     ino = getino(&mp->dev, pathname);
     //if ino doesn't exist
     if(!ino)
     {
       return 0;
     }
     mip = iget(mp->dev, ino);
     //if not a dir
     if(!S_ISDIR(mip->inode.i_mode))
     {
       printf("%s not a dir\n", pathname);
       iput(mip);
       return 0;
     }
     iput(running->cwd);
     running->cwd = mip;
    return 1;
}
//Stat
int do_stat(char* pathname, struct stat* mystat)
{
    int ino;
    MINODE* mip = running->cwd;
    
    //if pathname exists
    if(pathname)
    {
      ino = getino(&mip->dev, pathname);
    }
    else
    {
      ino = mip->ino;
    }
    
    //if no path return
    if(!ino)
    {
      return -1;
    }
    
    //update mip
    mip = iget(mip->dev, ino);
    
    //get stat props
    mystat->st_dev = mip->dev;
    mystat->st_ino = ino;
    mystat->st_mode = mip->inode.i_mode;
    mystat->st_nlink = mip->inode.i_links_count;
    mystat->st_uid = mip->inode.i_uid;
    mystat->st_gid = mip->inode.i_gid;
    mystat->st_size = mip->inode.i_size;
    mystat->st_blksize = BLKSIZE;
    mystat->st_blocks = mip->inode.i_blocks;
    mystat->st_atime = mip->inode.i_atime;
    mystat->st_mtime = mip->inode.i_mtime;
    mystat->st_ctime = mip->inode.i_ctime;
   
    
    /*//now print
    printf("------------- stat -----------\n");
    printf("dev = %6d\n", mystat->st_dev);
    printf("ino = %6d\n", mystat->st_ino);
    printf("mode = %5x\n", mystat->st_mode);
    printf("uid = %6d\n", mystat->st_uid);
    printf("gid = %6d\n", mystat->st_gid); 
    printf("nlink = %4d\n", mystat->st_nlink);
    printf("size = %6d\n", mystat->st_size);
    printf("time = %s", ctime(&mystat->st_ctime));*/


  iput(mip);
  return 1;
}
//get permissions for ls
char* permissionString(unsigned short mode)
{
  //temporary string
  char temp[BLKSIZE] = {0};
  //print all the possible outputs
  sprintf(temp, "%c%c%c%c%c%c%c%c%c", ((mode & (1<<8))? 'r':'-'), ((mode & (1<<7))? 'w':'-'), ((mode & (1<<6))? 'x':'-'), ((mode & (1<<5))? 'r':'-'), ((mode & (1<<4))? 'w':'-'), ((mode & (1<<3))? 'x':'-'), ((mode & (1<<2))? 'r':'-'), ((mode & (1<<1))? 'w':'-'), ((mode & (1))? 'x':'-'));
  
return temp;
}
//Print Working Directory
void pwd(void)
{ 
  MINODE *wd;
      wd = running->cwd;
   // 1. if wd is already the root:
   //    print /; return;
      if(wd->ino == ROOT_INODE)
      {
	printf("/");
      }
      else
      {
	do_pwd(wd->ino);
	
      }
      printf("\n");
}
int do_pwd(int ino)
{ 
  char buf[BLKSIZE];
  char parentBuf[BLKSIZE];
  char temp[BLKSIZE];
  int new, j = 0;
  char* cp;
  MINODE* pip;
  MINODE* mip;
  //if ino is real
  if(ino > 0)
  {
    //get my ip
    mip = iget(dev, ino);
    //if not root INODE
    if(mip->ino != 2)
    {
      //get it's block
      get_block(dev, mip->inode.i_block[0], buf);
      iput(mip);
      dp = (DIR*)buf;
      cp = buf;
      cp+= dp->rec_len;
      dp = (DIR*) cp;
      //keep going until you find root; recursion
      new = do_pwd(dp->inode);
    }
    //if root inode
    if(ino == 2)
    {
      return ino;
    }
    //get parent ip
    pip = iget(dev, new);
    j = 0;
    while(j<12)
    {
      //get block of parent inode
      get_block(dev, pip->inode.i_block[j], parentBuf);
      dp = (DIR*) parentBuf;
      cp = parentBuf;
      while(cp < &parentBuf[BLKSIZE])
      {
	//found the correct inode
	if(dp->inode == ino)
	{
	  j = 12;
	  //stop executing
	  break;
	}
	//update to next dir location
	cp+= dp->rec_len;
	dp = (DIR*) cp;
      }
      //next set of dirs
      j++;
    }
    //get the name of this dir
    strncpy(temp, dp->name, dp->name_len);
    temp[dp->name_len] = 0;
    printf("/%s", temp);
  }
  return 0;
}
//MAKE DIRECTORY
int make_dir(char pathname[])
{
  //string to store the child in
  char child[256];
  //string to store the parent
  char parent[256];
  //parent ip
  MINODE* pip;
  int ino = 0;
  int r = 0;
  int exists;
  PathToke(pathname, parent, child, n);
  //get IN_MEMORY minode of parent
  MINODE* mp = running->cwd;
  ino = getino(dev,parent);
  if(ino > 0)
  {
    
    pip = iget(dev, ino);
    // verify parent INODE is a DIR and Child does NOT exists in the parent directory
    exists = search(pip, child);
    //if returns 0 the desired dir doesn't exist
    if(exists == 0)
    {
      r = my_mkdir(pip, child);
      running->cwd = iget(dev, running->cwd->ino);
    }
  }
  return r;
} 
int my_mkdir(MINODE* pip, char* name)
{
  unsigned long inumber = 0, bnumber = 0, newnumber = 0;
  MINODE* mip;
  int j = 0, holder;
  char* cp;
  char buf[BLKSIZE], cbuf[BLKSIZE], dbuf[BLKSIZE], namebuf[BLKSIZE];
  int done = 0;
  int need_length = 0;
  int ideal_length = 0;
  //allocated an inode and diskblock for the new directroy
  //call inumber = ialloc(dev) and bnumber = balloc(dev);
  inumber = ialloc(pip->dev);
  bnumber = balloc(pip->dev);
  //call mip = iget(dev, inumber) to load the inode into a minode
  mip = iget(pip->dev, inumber);
  //write contents into mip->INODE o make it as a DIR_MODE
  mip->inode.i_mode = 0x41ED;
  mip->inode.i_uid = running->uid;
  mip->inode.i_gid = running->gid;
  mip->inode.i_size = BLKSIZE;
  mip->inode.i_links_count = 2;
  mip->inode.i_atime = mip->inode.i_ctime = mip->inode.i_mtime = time(0L);
  mip->inode.i_blocks = 2;
  mip->dirty = 1;
  mip->inode.i_block[0] = bnumber;
  for(j = 1; j < 15; j++)
  {
    mip->inode.i_block[j] = 0;
  }
  //put mip back to memory
  iput(mip);
  //write (.) and (..) entries into a buf and write buf to the disk block allocated to this directory
  //start with (.)
  dp = (DIR*) buf;
  dp->inode = inumber;
  dp->file_type = 2;
  strncpy(dp->name, ".", 1);
  dp->name_len = 1;
  dp->rec_len = 12;
  cp = buf+12;
  dp = (DIR*)cp;
  //now (..)
  dp->inode = pip->ino;
  dp->name_len = 2;
  dp->file_type = 2;
  strncpy(dp->name, "..", 2);
  dp->rec_len = BLKSIZE - 12;
  //put the block back
  put_block(dev, bnumber, buf);
  j = 0;
  //parent trap time
  while(j < 12 && done == 0)
  {
//    get_block(dev, pip->inode.i_block[j], dbuf);
    lseek(dev, (long)(pip->inode.i_block[j] * BLKSIZE), 0);
    read( dev, dbuf, BLKSIZE);
    dp = (DIR*) dbuf;
    cp = dbuf;
    //entering a new entry 
    need_length = 4*((11+strlen(name))/4);
    //step to the last entry
    if(pip->inode.i_block[j] > 0)
    {
      while(cp < &dbuf[BLKSIZE])
      {
	
	cp += dp->rec_len;
	if (cp >= &dbuf[BLKSIZE])
	{	
		break;
	}
	dp = (DIR*) cp;
      }
      strncpy(namebuf, dp->name, dp->name_len);
      namebuf[dp->name_len] = 0;
      ideal_length = 4*((11+ strlen(namebuf))/4);
      //hold onto this value so we can adjust length
      holder = dp->rec_len;
    }
    //if reclen - ideal >= need length then enter as last entry and trim it down
    if(((holder - ideal_length) >= need_length)&&(pip->inode.i_block[j] > 0))
    {
      //adjust dp to ideal length
      dp->rec_len = ideal_length;
      cp -= holder;
      dp = (DIR*)cp;
      cp+= dp->rec_len;
      dp = (DIR*)cp;
      dp->inode = inumber;
      strncpy(dp->name,name, strlen(name));
      dp->name_len = strlen(name);
      dp->rec_len = holder - ideal_length;
      dp->file_type = 2;
      //write parents block back to disk
      put_block(dev, pip->inode.i_block[j], dbuf);
      done = 1;
    }
    //allocate a new data block. enter as first data block
    else if(pip->inode.i_block[j] == 0)
    {
      newnumber = balloc(dev);
      get_block(dev, newnumber, cbuf);
      dp = (DIR*) cbuf;
      dp->inode = inumber;
      dp->file_type = 2;
      dp->name_len = strlen(name);
      strncpy(dp->name, name, dp->name_len);
      dp->rec_len = BLKSIZE;
      pip->inode.i_block[j] = newnumber;
      pip->inode.i_size += BLKSIZE;
      //write parents block back to disk
      //put_block(dev, pip->inode.i_block[j], cbuf);
      lseek(dev, (long)(pip->inode.i_block[j]*BLKSIZE), 0);
      write( dev, cbuf, BLKSIZE);
      done = 2;
    }
    j++;
  }
  //mark as dirty
  pip->dirty = 1;
  //update i_links_count
  pip->inode.i_links_count++;
  //touch a time and mark as dirty
  pip->inode.i_atime = pip->inode.i_mtime = time(0L);
  //put away your blocks like a good little asshole
  iput(pip);
  return done;
}
//MAKE FILE
int creat_file(char* pathname)
{
  //string to store the child in
  char child[256];
  //string to store the parent
  char parent[256];
  //parent ip
  MINODE* pip;
  int ino = 0;
  int r = 0;
  int exists;
  PathToke(pathname, parent, child, n);
  //get IN_MEMORY minode of parent
  MINODE* mp = running->cwd;
  ino = getino(&mp->dev,parent);
  if(ino > 0)
  {
    pip = iget(dev, ino);
    // verify parent INODE is a DIR and Child does NOT exists in the parent directory
    exists = search(pip, child);
    //if returns 0 the desired dir doesn't exist
    if(exists == 0)
    {
      r = my_creat(pip, child);
      running->cwd = iget(dev, running->cwd->ino);
    }
  }
  return r;
} 
int my_creat(MINODE* pip, char* name)
{
  unsigned long inumber = 0, bnumber = 0, newnumber = 0;
  MINODE* mip;
  int j = 0, holder;
  char* cp;
  char buf[BLKSIZE], cbuf[BLKSIZE], dbuf[BLKSIZE], namebuf[BLKSIZE];
  int done = 0;
  int need_length = 0;
  int ideal_length = 0;
  //allocated an inode and diskblock for the new directroy
  //call inumber = ialloc(dev) and bnumber = balloc(dev);
  inumber = ialloc(dev);
  bnumber = ialloc(dev);
  //call mip = iget(dev, inumber) to load the inode into a minode
  mip = iget(dev, inumber);
  //write contents into mip->INODE o make it as a DIR_MODE
  mip->inode.i_mode = 0x81a4;
  mip->inode.i_uid = running->uid;
  mip->inode.i_gid = running->gid;
  mip->inode.i_size = BLKSIZE;
  mip->inode.i_links_count = 1;
  mip->inode.i_atime = mip->inode.i_ctime = mip->inode.i_mtime = time(0L);
  mip->inode.i_blocks = 2;
  mip->dirty = 1;
  mip->inode.i_block[0] = bnumber;
  for(j = 1; j < 15; j++)
  {
    mip->inode.i_block[j] = 0;
  }
  //put mip back to memory
  iput(mip);
  j = 0;
  //parent trap time
  while(j < 12 && done == 0)
  {
    get_block(dev, pip->inode.i_block[j], dbuf);
    dp = (DIR*) dbuf;
    cp = dbuf;
    //entering a new entry 
    need_length = 4*((11+strlen(name))/4);
    //step to the last entry
    if(pip->inode.i_block[j] > 0)
    {
      while(cp < &dbuf[BLKSIZE])
      {
	dp = (DIR*) cp;
	cp += dp->rec_len;
      }
      strncpy(namebuf, dp->name, dp->name_len);
      namebuf[dp->name_len] = 0;
      ideal_length = 4*((11+ strlen(namebuf))/4);
      //hold onto this value so we can adjust length
      holder = dp->rec_len;
    }
    //if reclen - ideal >= need length then enter as last entry and trim it down
    if((holder - ideal_length >= need_length)&&(pip->inode.i_block[j] > 0))
    {
      //adjust dp to ideal length
      dp->rec_len = ideal_length;
      cp -= holder;
      dp = (DIR*)cp;
      cp+= dp->rec_len;
      dp = (DIR*)cp;
      dp->inode = inumber;
      strncpy(dp->name,name, strlen(name));
      dp->name_len = strlen(name);
      dp->rec_len = holder - ideal_length;
      dp->file_type = 2;
      //write parents block back to disk
      put_block(dev, pip->inode.i_block[j], dbuf);
      done = 1;
    }
    //allocate a new data block. enter as first data block
    else if(pip->inode.i_block[j] == 0)
    {
      newnumber = balloc(dev);
      get_block(dev, newnumber, cbuf);
      dp = (DIR*) cbuf;
      dp->inode = inumber;
      dp->file_type = 2;
      dp->name_len = strlen(name);
      strncpy(dp->name, name, dp->name_len);
      dp->rec_len = BLKSIZE;
      pip->inode.i_block[j] = newnumber;
      pip->inode.i_size += BLKSIZE;
      put_block(dev, pip->inode.i_block[j], cbuf);
      done = 2;
    }
    j++;
  }
  //mark as dirty
  pip->dirty = 1;
  pip->inode.i_atime = pip->inode.i_mtime = time(0L);
  iput(pip);
  return done;
}
//REMOVE DIRECTORY
//still needs work, double check algorithm
int rm_dir(char* pathname)
{
	char parent[BLKSIZE];
	char child[BLKSIZE];
	char dbuf[BLKSIZE];
	char name[BLKSIZE];
	int ino, r = 0, i;
	int exists = 0;
	char* cp;
	char* tmp;
	MINODE* mip;
	MINODE* pip;
	//get its ino
	MINODE* mp = running->cwd;
	//tmp = strdup(pathname);
	ino = getino(dev, pathname);
	if(ino == running->cwd->ino)
	{
		return -1;
	}
	if(ino > 0)
	{
		//get a pointer to it's minode[]
		mip = iget(dev, ino);
		//check if DIR type type && not busy && is empty
		if((mip->inode.i_mode & 0040000) != 0040000 || mip->inode.i_links_count > 2 || mip->refCount > 1 || mip->inode.i_size > BLKSIZE)
		{	
			return -1;
		}
		else
		{	
	          PathToke(pathname, parent, child, n);
		  //assume passed above checks	
		  int z = 0;
		  for(z = 0; z<12; z++)
		  {
		    if(mip->inode.i_block[z] == 0)
		    {	
		    	continue;
		    }
			
		    bdalloc(mip->dev, mip->inode.i_block[z]);
		  }
		  idalloc(mip->dev,mip->ino);
		  iput(mip);
		  ino = getino(mp->dev, parent);
		  if(ino > 0)
		  {
		    //get parent	
		    pip = iget(dev, ino);
		    //see if exists
		   
		    exists = search(pip, child);
// printf("exists : %d\n", exists);
		    if(exists != 0)
		    {
		      //remove child
		//printf("at rm child\n");
		      r = rm_child(pip, child);
//printf("past rm child, r=%d\n", r);
		      //decrement pip's link count by 1
		      pip->inode.i_links_count--;
		      //touch pips atime mtime fields
		      do_touch(parent);
		      //mark pip dirty
		      pip->dirty = 1;
		      //iput pip
		      iput(pip);
		    }
		    
		  }
		  
		 }
	  running->cwd = iget(dev, running->cwd->ino); 
	}
	return r;
}
int rm_child(MINODE* parent, char* my_name)
{
	int j = 0, store;
	char* cp;
	char* newCP;
	int ino, holder, done = 0;
	char dbuf[BLKSIZE], name[BLKSIZE];
	DIR* newDP;
	//search parent INODEs data block(s) for the entry of my_name
	while(j < 12)
	{
	    if(parent->inode.i_block[j] > 0)
	    {
		get_block(dev, parent->inode.i_block[j], dbuf);
		dp = (DIR*) dbuf;
		cp = dbuf;
		while(cp < &dbuf[BLKSIZE])
		{
			strncpy(name, dp->name, dp->name_len);
			name[dp->name_len] = 0;
			if(strcmp(name, my_name) == 0)
			{
				done = 1;
				break;
			}
			//keep the spot before
			store = dp->rec_len;
			cp+= dp->rec_len;
			dp = (DIR*) cp;
		}
	    }
		if(done == 1)
		{
			break;
		}
		j++;
	}
	//erase my_name from parent directory by
	//if not first entry in block
	if(dp->rec_len == BLKSIZE)
	{
		bdalloc(parent->dev, parent->inode.i_block[j]);
		parent->inode.i_block[j] = 0;
		parent->inode.i_size -= BLKSIZE;
	}
	//if entry first in a data block
	//update all relevant locations
	else if ( dp->rec_len < BLKSIZE )
	{
		holder = dp->rec_len;
		if ( cp + dp->rec_len >= &dbuf[BLKSIZE])
		{
			cp -= store;
			dp = (DIR *)cp;
			dp->rec_len += holder;
		}
		else

		{
			newCP = cp + dp->rec_len;
			newDP = (DIR *)newCP;
			while (newCP < &dbuf[BLKSIZE])
			{
				strncpy(name, newDP->name, newDP->name_len);
				name[newDP->name_len] = 0;
				strncpy(dp->name, name, newDP->name_len);
				dp->name_len = newDP->name_len;
				dp->file_type = newDP->file_type;
				dp->inode = newDP->inode;
				dp->rec_len = newDP->rec_len;
				if (newCP + newDP->rec_len >= &dbuf[BLOCK_SIZE])
				{
					break;
				}
				newCP += newDP->rec_len;
				newDP = (DIR *)newCP;
				cp += dp->rec_len;
				dp = (DIR *)cp;
			}
			dp->rec_len = newDP->rec_len + holder;
			dp->name_len = newDP->name_len;
			dp->file_type = newDP->file_type;
			dp->inode = newDP->inode;
			newCP-=newDP->rec_len;
		}
	}		
	//deallocate the data block and modify its parent's file size
	//write parent data block back to disk, makre parent minode DIRTY
	put_block(parent->dev, parent->inode.i_block[j], dbuf);
	return done;
}
//TOUCH
int do_touch(char *pathname)
{
	MINODE *pip;	
	int ino = 0;
	MINODE* mp = running->cwd;
	ino = getino(&mp->dev, pathname);
	if (ino > 0)
	{
		pip = iget(dev, ino);			
		pip->dirty = 1;		
		pip->inode.i_atime = pip->inode.i_mtime = time(0L);
		iput(pip);
		return 1;
	}
	return 0;
}
//LINK FILE
int link(char oldFileName[], char newFileName[])
{
  //creates a newFileName which has the SAME inode number of oldfile
  char* newFile;
  char* newParent;
  char oldBase[BLKSIZE], oldChild[BLKSIZE];
  char newBase[BLKSIZE], newChild[BLKSIZE];
  MINODE* newPip;
  MINODE* oldMip;
  
  int oldIno = 0, newIno = 0, exists = 0, r = 0;
  //make sure both file names exist
  if(oldFileName != NULL && NULL != newFileName)
  {
    
    //get base and dirname
    PathToke(oldFileName, oldBase, oldChild, n);
    PathToke(newFileName, newBase, newChild, n);
   //get the inode of oldFileName into memory mip->minode[]
    oldIno = getino(dev,oldFileName);
    newIno = getino(dev, newBase); 
    if((oldIno > 0)&&(newIno > 0))
    {
      newPip = iget(dev, newIno);
      oldMip = iget(dev, oldIno);
      //check that oldFileName is a REG file and newFileName base exists but not name
      exists = search(newPip, newChild);
      if((exists == 0)&&((oldMip->inode.i_mode & 0100000)==0100000))
      {

	r = my_link(newPip, oldMip, newChild);
      }
    }
    running->cwd = iget(dev, running->cwd->ino);
  }
  return r;
}
int my_link(MINODE* newPip, MINODE* oldMip, char* newFile)
{	
	unsigned long inumber = 0, newnumber = 0;
	int j = 1, holder; 
	char* cp;
	int need_length = 0, ideal_length = 0;
	int done = 0;
	char buf[BLKSIZE], dbuf[BLKSIZE], cbuf[BLKSIZE], name[256];
	j = 0;
	//parent trap time	
	while ( j < 12 && done == 0 )
	{
		get_block(dev, newPip->inode.i_block[j],dbuf);
		dp = (DIR *)dbuf;
		cp = dbuf;
		need_length = 4*((11+strlen(newFile))/4);
		if (newPip->inode.i_block[j] > 0)
		{
			while (cp < &dbuf[BLOCK_SIZE])
			{
				dp = (DIR*) cp;
				cp += dp->rec_len;
			}
			strncpy(name, dp->name, dp->name_len);
			name[dp->name_len] = 0;
			ideal_length  = 4*((11+strlen(name))/4);
			//hold onto this value so we can adjust length
			holder = dp->rec_len;			
		}
		//if reclen - ideal >= need length then enter as last entry and trim it down
		if(((holder - ideal_length) >= need_length)&&(newPip->inode.i_block[j] > 0))
		{	
			dp->rec_len = ideal_length;
			cp -= holder;
			dp = (DIR *)cp;
			cp += dp->rec_len;
			dp = (DIR *)cp;
			dp->inode = oldMip->ino;
			dp->name_len = strlen(newFile);
			strncpy(dp->name, newFile, dp->name_len);
			dp->rec_len = holder - ideal_length;
			dp->file_type = 0;
			//write parents block back to disk
			put_block(dev, newPip->inode.i_block[j], dbuf);	
			done = 1;
		}
		//allocate a new data block. enter as first data block
		else if (newPip->inode.i_block[j] == 0)
		{
			newnumber = balloc(dev);
			get_block(dev, newnumber, cbuf);
			dp = (DIR *)cbuf;
			dp->inode = oldMip->ino;
			dp->file_type = 0;
			dp->name_len = strlen(newFile);
			strncpy(dp->name, newFile, dp->name_len);
			dp->rec_len = BLKSIZE;
			newPip->inode.i_block[j] = newnumber;	
			//write parents block back to disk
			put_block(dev, newPip->inode.i_block[j], cbuf);;
			done = 2;
		}
		j++;		
	}
	//mark as dirty
	newPip->dirty = 1;
	oldMip->dirty = 1;
	//update new links count
	oldMip->inode.i_links_count++;
	//touch time
	oldMip->inode.i_atime = oldMip->inode.i_mtime = time(0L);
	newPip->inode.i_atime = newPip->inode.i_mtime = time(0L);
	//put away your blocks like a good little asshole
	iput(oldMip);
	iput(newPip);
	return done;
}
//UNLINK FILE
int unlink(char *pathname)
{

//(1). get /a/b/c's INODE into memory

//(2). verify it's a FILE (not a DIR);

//(3). decrement INODE's i_links_count by 1;

    int ino = 0;
    int r=-1;
    MINODE* mip;
    MINODE* pip;
    struct stat mystat;
    char* childkiller = pathname;
    char* child=basename(childkiller);
    char* parent;
    int i = 0, exists = 0;
    ino = getino(dev, pathname);
	
    //if no path return fail
    if(!ino)
    {
      return -1;
    }

    //update mip
    mip = iget(dev, ino);

    do_stat(pathname, &mystat);

    if((mystat.st_mode & 0100000) != 0100000)
    {
        printf("\nnot a regular file!\n");
    	return -1; //return fail
    }
    //decrement inode links count by one
    mip->inode.i_links_count--;
 

//(4). if i_links_count == 0 ==> rm /a/b/c
//        deallocate its data blocks;
//        deallocate its INODE
	if ( mip->inode.i_links_count == 0 )
	{
		for (i = 0; i < 12; i++)
		{
			if ( mip->inode.i_block[i] == 0 )
				continue;
			bdalloc(mip->dev, mip->inode.i_block[i]);			
		}
		idalloc(mip->dev, mip->ino);
	}
        mip->dirty = 1;
	iput(mip);
//(5). Remove c from the parent directory /a/b/ by
 //       rm_child(parentInodePtr, childName)
 //    which is the SAME as that in rmdir or rm file operations.
    	//get the parent ino
	char* tmp;
	tmp = strdup(pathname);
	parent = dirname(tmp);
	tmp = strdup(pathname);
	child = basename(tmp);
	ino = getino(dev, parent);
		
    	//if no path return fail
    	if(!ino)
    	{
     		 return -1;
    	}
	//get parent pip
	pip = iget(dev, ino);
	//see if aprents exist
	exists = search(pip, child);
	if(exists == 1)
	{	// remove child	
		r=rm_child(pip, child);
		//touch parent
		do_touch(parent);
		pip->dirty = 1;
		iput(pip);
	}
    running->cwd = iget(dev, running->cwd->ino);		
    return r;//success
}
//SYMBOLIC LINK
int my_symlink(char oldFileName[], char newFileName[])
{
  //creates a newFileName which has the SAME inode number of oldfile
  char* newFile;
  char* newParent;
  char oldBase[BLKSIZE], oldChild[BLKSIZE];
  char newBase[BLKSIZE], newChild[BLKSIZE];
  MINODE* newPip;
  MINODE* oldMip;
  MINODE* newMip;
  
  int oldIno = 0, newIno = 0, exists = 0, r = 0;
  
  // make sure names exist
  if(NULL != oldFileName && NULL != newFileName)
  {
    //verify oldName exists(either a dir or file)
    //get base and dirname
    PathToke(oldFileName, oldBase, oldChild, strlen(oldFileName));
    PathToke(newFileName, newBase, newChild, strlen(newFileName));
   //get the inode of oldFileName into memory mip->minode[]
    oldIno = getino(dev,oldFileName);
    newIno = getino(dev, newBase); 
    if((oldIno > 0)&&(newIno > 0))
    {
      newPip = iget(dev, newIno);
      oldMip = iget(dev, oldIno);
      //check that oldFileName is a REG file and newFileName base exists but not name
      exists = search(newPip, newChild);
      if(exists == 0)
      {
	//create a file
	creat_file(newFileName);
	newIno = getino(dev, newFileName);
	if(newIno > 0)
	{
	  newMip = iget(dev, newIno);
	  //change S_IFLNK(120000) = 0xA
	  r = newMip->inode.i_mode |= 0120000;
	  strcpy(newMip->inode.i_block, oldChild);
	  //mark as dirty
	  newMip->dirty = 1;
	  newMip->refCount = 1;
	  //put your blocks away
	  iput(newMip);
	}
      }
      running->cwd = iget(dev, running->cwd->ino);
    }
  }
}

//CHANGE PERMISSIONS
int my_chmod(char newPermission[], char pathname[])
{
	char* tmp, key[] = "augo+-rwx", *pch;
	MINODE* pip;
	MINODE* mp = running->cwd;
	int ino = 0;
	//list the field permission variables
	int read = 0, write = 0, append = 0, operator = 0, other = 0, group = 0, user = 0;
	
	//check if at root, can't modifu it.
	if(((pathname == NULL) && (mp->ino == 2)) || strcmp(pathname, "/") == 0) 
	{
		printf("Invalid: cannot chmod this path\n");
		return 0;
	}
	//get the ino at this pathname
	ino = getino(mp->dev, pathname);
	//if ino exists
	if(ino > 0)
	{
		//get the block of the parent
		pip = iget(mp->dev, ino);
		//mark as dirty 
		pip->dirty = 1;
		//find anything in key by comparing and looping
		pch = strpbrk(newPermission, key);
		while (pch != NULL)
		{	
			//u = user or owner of file
			//g = group or users who are members of file's group
			//o = others or users who are neither the owner of the file not members of the file's group
			//a = all or all thre of the above
			//+ = adds spevified modes to the classes
			//- = removes all the specified modes from the classes
			//r = read or read a file or list directory contents
			//w = write or write to a file
			//x = execute a file or recurse a dir tree
			
			//if an a is found mark all as 1
			if ( *pch == 'a' )
				other = group = user = 1;
			//if g is found mark group as 1
			else if ( *pch == 'g' )
				group = 1;
			//if o is found mark other as 1
			else if ( *pch == 'o' )
				other = 1;
			//if u is found mark user as 1
			else if ( *pch == 'u' )
				user = 1;
			//if + is found mark operator as 1
			else if ( *pch == '+' )
				operator = 1;
			//if - is found mark operator as 2
			else if ( *pch == '-' )
				operator = 2;
			//if r is found mark read as 1
			else if ( *pch == 'r' )
				read = 1;
			//if w is found mark write as 1
			else if ( *pch == 'w' )
				write = 1;
			//if x is found mark append as 1
			else if ( *pch == 'x' )
				append = 1;
			//there are no other permision found
			else
			{
				printf("INVALID OPERATOR: %s\n", pch);
				return 0;
			}
			//update current permission check to next location and check against key
			pch = strpbrk (pch+1,key);
		}	
		//if user is found
		if(user == 1)
		{
			//check if read write or append are 1
			if(read == 1)
			{
				// (+) adds the specified modes to the classes
				if(operator == 1)
				{
					//add
					pip->inode.i_mode = pip->inode.i_mode | (1 << 8);
				}
				//(-) removes specified modes to the classes
				else if(operator == 2)
				{
					//negate
					pip->inode.i_mode = pip->inode.i_mode & ~(1 << 8);
				}
			}
			if(write == 1)
			{
				// (+) adds the specified modes to the classes
				if(operator == 1)
				{
					//add
					pip->inode.i_mode = pip->inode.i_mode | (1 << 7);
				}
				//(-) removes specified modes to the classes
				else if(operator == 2)
				{
					//negate
					pip->inode.i_mode = pip->inode.i_mode & ~(1 << 7);
				}
			}
			//check if read write or append are 1
			if(append == 1)
			{
				// (+) adds the specified modes to the classes
				if(operator == 1)
				{
					//add
					pip->inode.i_mode = pip->inode.i_mode | (1 << 6);
				}
				//(-) removes specified modes to the classes
				else if(operator == 2)
				{
					//negate
					pip->inode.i_mode = pip->inode.i_mode & ~(1 << 6);
				}
			}
		}
		if( group == 1)
		{
			//check if read write or append are 1
			if(read == 1)
			{
				// (+) adds the specified modes to the classes
				if(operator == 1)
				{
					//add
					pip->inode.i_mode = pip->inode.i_mode | (1 << 5);
				}
				//(-) removes specified modes to the classes
				else if(operator == 2)
				{
					//negate
					pip->inode.i_mode = pip->inode.i_mode & ~(1 << 5);
				}
			}
			//check if read write or append are 1
			if(write == 1)
			{
				// (+) adds the specified modes to the classes
				if(operator == 1)
				{
					//add
					pip->inode.i_mode = pip->inode.i_mode | (1 << 4);
				}
				//(-) removes specified modes to the classes
				else if(operator == 2)
				{
					//negate
					pip->inode.i_mode = pip->inode.i_mode & ~(1 << 4);
				}
			}
			//check if read write or append are 1
			if(append == 1)
			{
				// (+) adds the specified modes to the classes
				if(operator == 1)
				{
					//add
					pip->inode.i_mode = pip->inode.i_mode | (1 << 3);
				}
				//(-) removes specified modes to the classes
				else if(operator == 2)
				{
					//negate
					pip->inode.i_mode = pip->inode.i_mode & ~(1 << 3);
				}
			}
		}
		if(other == 1)
		{
			//check if read write or append are 1
			if(read == 1)
			{
				// (+) adds the specified modes to the classes
				if(operator == 1)
				{
					//add
					pip->inode.i_mode = pip->inode.i_mode | (1 << 2);
				}
				//(-) removes specified modes to the classes
				else if(operator == 2)
				{
					//negate
					pip->inode.i_mode = pip->inode.i_mode & ~(1 << 2);
				}
			}
			//check if read write or append are 1
			if(write == 1)
			{
				// (+) adds the specified modes to the classes
				if(operator == 1)
				{
					//add
					pip->inode.i_mode = pip->inode.i_mode | (1 << 1);
				}
				//(-) removes specified modes to the classes
				else if(operator == 2)
				{
					//negate
					pip->inode.i_mode = pip->inode.i_mode & ~(1 << 1);
				}
			}
			//check if read write or append are 1
			if(append == 1)
			{
				// (+) adds the specified modes to the classes
				if(operator == 1)
				{
					//add
					pip->inode.i_mode = pip->inode.i_mode | (1 << 0);
				}
				//(-) removes specified modes to the classes
				else if(operator == 2)
				{
					//negate
					pip->inode.i_mode = pip->inode.i_mode & ~(1 << 0);
				}
			}
		}
		//update the time at this path
		do_touch(pathname);
		//put blocks away 
		iput(pip);
		//update running->cwd
		running->cwd = iget(mp->dev, mp->ino);
		return 1;
		}
	return 0;
}
//OPEN FILE
int open_file(char pathname[], char mode[])
{
	int newMode = -1, ino = 0, check = -1, i = 0;
	MINODE* mip = malloc(sizeof(MINODE));
	MINODE* mp = running->cwd;
	char* temp;
	char* child;
	OFT* oftp = malloc(sizeof(OFT));
	
	oftp->refCount = 0;

	//2. get pathname's inumber: ino = getino(&dev, pathname);
	ino = getino(dev,pathname);
	if(ino > 0)
	{

		//3. get its Minode pointer mip = iget(dev,ino);  
		mip = iget(mp->dev, ino);
		//4. check mip->INODE.i_mode to verify it's a REGULAR file and permission OK.
		// (Optional : do NOT check FILE type so that we can open DIRs for RW)
		if(((mip->inode.i_mode & 0100000) == 0100000) && mip->inode.i_uid == running->uid)
		{
			//Check whether the file is ALREADY opened with INCOMPATIBLE type:
            //If it's already opened for W, RW, APPEND : reject.
            //(that is, only multiple R are OK)
			newMode = ModeConvert(mode);
			//check = FileCheck(mip, newMode);
//printf("HERE %d\n", check);
			//if ( check > -1 )
			//{
				//5. allocate an OpenFileTable (OFT) entry and fill in values:
				//oftp = falloc();       // get a FREE OFT
				oftp = falloc(check);
				//oftp->mode = mode;     // open mode 
				oftp->mode = newMode;
				//oftp->refCount = 1;
				oftp->refCount++;
				//oftp->inodeptr = mip;  // point at the file's minode[]
				oftp->inodeptr = mip;

				//6. Depending on the open mode 0|1|2|3, set the OFT's offset accordingly:
		        //switch(mode){
				switch(newMode)
				{
				case 0:
					oftp->offset = 0;
					break;
				case 1:
					// W : truncate file to 0 size     
					oftp->offset = 0;
					truncate(mip);   
					break;
				case 2: 
					// RW does NOT truncate file
					oftp->offset = 0;   
					break;
				case 3:
					// APPEND mode
					oftp->offset =  mip->inode.i_size;  
					break;
				default: 
					printf("invalid mode\n");
					return(-1);
				}
				//7. find the SMALLEST i in running PROCs fd[] such that fd[i] is 					//null
				int j;
				for(j = 0; j < NFD; j++)
				{
					if(running->fd[j] == NULL)
					{
						running->fd[j] = oftp;
						break;
					}
				}		
				//8. update INODE's time field.
				//add name to oftp
				//for W|RW|APPEND mode : mark Minode[] dirty
				if(newMode > 0)
				{
					oftp->inodeptr->inode.i_atime = oftp->inodeptr->inode.i_mtime = time(0L);
				}
				child = basename(pathname);
				strcpy(oftp->name, child);
			//}
			oftp->inodeptr->dirty = 1;
		}
	}
    //9. return i as the file descriptor
	return check;
}
int truncate(MINODE *mip)
{
	int i = 0;
	//search through all the mip
	while ( i < 12 )
	{
		//check the blocks
		if (mip->inode.i_block[i] > 0)
		{
			//deallocate
			bdalloc(dev, mip->inode.i_block[i]);
			mip->inode.i_block[i] = 0;				
		}
		//update
		i++;
	}
	i = DTbdalloc(mip);
	//set size to zero
	mip->inode.i_size = 0;
	return i;
}
int ModeConvert (char* mode)
{
	int number = -1;	
	//check mdoe against the different file types
	if ( strncmp(mode, "RW", 2) == 0 )
	{
		number = 2;
	}
	else if ( strncmp(mode, "R", 1) == 0 )
	{	
		number = 0;
	}
	else if ( strncmp(mode, "W", 1) == 0 )
	{
		number = 1;
	}
	else if ( strncmp(mode, "APPEND", 6) == 0 )
	{
		number = 3;
	}
	else
	{
		number = -1;
	}
	//return choice
	return number;
}
int *FileCheck(MINODE* mip, int mode)//issue for open file
{	
	int i = 0;
	//check all 10 NFD
	while (i < NFD)
	{
		//check if at end
		if(running->fd[i] == 0)
		{
			return i;
		}
		if (running->fd[i]->refCount > 0)
		{
			if (mip->ino == running->fd[i]->inodeptr->ino)
			{
				printf("here %d\n",i);				
				return i;
			}
		}
		i++;
	}
	i = 0;
	//find one with refcount equal to zero if can't find appropriate one
	while ( i < NFD )
	{
		//check if at end
		if(running->fd[i] == 0)
		{
			return i;
		}
		if ( running->fd[i]->refCount == 0 )
		{
			printf("here %d\n",i);
			return i;
		}
		i++;
	}
	return 10;	
}
int close_file(int fd)
{
	OFT* oftptr;
	MINODE* mip;
	//check if fd is in range
	if ( fd > -1 && fd < 10 )
	{
		//verify running->fd[fd] is pointing at a OFT entry
		if (running->fd[fd] != 0)
		{
			//the following code segment should be fairly obvious
			oftptr = running->fd[fd];
			oftptr->refCount--;
			if (oftptr->refCount > 0)
			{
				return 0;
			}
			//last user of this OFT entry ==> dispose of the Minode[]
			mip = malloc(sizeof(MINODE));			
			mip = oftptr->inodeptr;
			iput(mip);
			fdalloc(oftptr);
			return 0;
		}
		return -1;
	}
	return -2;
}
void pfd(void)
{
	int i = 0, ino = 0;
	printf("fd  mode  count offset [dev,ino] filename\n");
	printf("-- ------ ----- ------ --------- --------\n");
	//check the open file table
	for (i = 0; i < 10; i++)
	{	
		//check if at end
		if(running->fd[i] == 0)
		{
			break;
		}
		if (running->fd[i]->refCount > 0)
		{
			//get the running fds
			ino = running->fd[i]->inodeptr->ino;
			//print fd
			printf("%d  ", i);
			//print mode
			if ( running->fd[i]->mode == 0 )
				printf(" READ  ");
			else if ( running->fd[i]->mode == 1 )
				printf(" WRITE ");
			else if ( running->fd[i]->mode == 2 )
				printf("  R/W  ");
			else if ( running->fd[i]->mode == 3 )
				printf("APPEND ");
			//print the refcount offsset and dev/ino #
			printf("  %d	%d	 %d  %d ", running->fd[i]->refCount, running->fd[i]->offset, running->fd[i]->inodeptr->dev, running->fd[i]->inodeptr->ino);
			//print the filename
			printf("	 %s\n", running->fd[i]->name);
		}
	}
}	
long read_file(int fd, int nbytes, int print )
{
	char buf[BLOCK_SIZE];
	long copied;
	int i = 0;
	if ( running->fd[fd]->refCount > 0 && ( running->fd[fd]->mode == 0 	|| running->fd[fd]->mode == 2 ))
	{
		copied = my_read(fd, buf, nbytes);
		if ( copied < BLKSIZE )
			for (i = copied; i < BLKSIZE; i++)
				buf[i] = 0;
		if ( print == 0 )	
		{	
			printf("-----------Contents Read-----------\n");
			printf("%s\n", buf);	
			printf("%lu actually read\n", copied);
		}
		else
			printf("%s", buf);		
		return copied;
	}
	else 
	{
		printf("READ FAILED\n");
		return 0;
	}
}

long my_read(int fd, char *buf, int nbytes)
{
	long size = running->fd[fd]->inodeptr->inode.i_size - running->fd[fd]->offset, dInBlock[256], InBlock[256], midBlock = 0;
	OFT *oftp = running->fd[fd];
	int lbk = 0, startByte = 0, block = 0, remain = 0, count = 0;
	MINODE *mip = running->fd[fd]->inodeptr;
	char readBuf[BLKSIZE], *cp, *cq;
	while ( nbytes > 0 && size > 0 && count < BLKSIZE )
	{	
		//compute logical block and offset
		lbk = oftp->offset / BLKSIZE;
		startByte = oftp->offset % BLKSIZE;
		//get direct blocks
		if ( lbk < 12 )
			block = mip->inode.i_block[lbk];
		//get indirect blocks
		else if (lbk >= 12 && lbk < 12 + 256 )
		{
			get_block(dev, mip->inode.i_block[12], dInBlock);
			block = dInBlock[lbk - 12];
		}
		//get double indirect blocks
		else
		{
			get_block(dev, mip->inode.i_block[13], dInBlock);
			midBlock = dInBlock[(lbk - (12 + 256))/256];
			if (midBlock != 0)
			{
				get_block(dev, midBlock, InBlock);
				block = InBlock[(lbk - (12 + 256))%256];
			}
		}
		//get data block into readbuf
		get_block(dev, block, readBuf);
		//copy from start buf to byte
		cp = readBuf + startByte;
		cq = buf;
		remain = BLKSIZE - startByte;

		while (remain > 0)
		{
			*cq++ = *cp++;            // cq points at buf[ ]       
			oftp->offset++;
			count++;       
			size--; nbytes--;  remain--;
			if (nbytes <= 0 || size <= 0 ) 
				break;
		
		}
	}
	return count;
}
//need to work on how to do proper indirect and double indirect. Segment fault
long write_file(int fd, char *text, int print)
{
	long written = 0, nbytes = 0; 
	int i = 0;
	char buf[BLOCK_SIZE];
	if (running->fd[fd]->refCount > 0 &&(running->fd[fd]->mode == 1 || running->fd[fd]->mode == 2 || running->fd[fd]->mode == 3 ))
	{
		if ( strlen(text) < BLKSIZE )
		{		
			strncpy(buf, text, strlen(text));
			buf[strlen(text)] = '\0';	
			nbytes = strlen(text);
		}
		else
		{
			strncpy(buf, text, BLKSIZE);
			nbytes = BLKSIZE;
		}
		if ( nbytes > 0 )
			written = my_write(fd, buf, nbytes);		
		else
			printf("YOU FORGOT TEXT\n");
		
		if ( print == 1 )
		{			
			printf("%s", buf);
			printf("wrote %d char into file fd=%d\n", written, fd);
		}
		return written;
	}
	else 
	{
		printf("WRITE FAILED\n");
		return 0;
	}
}
long my_write(int fd, char *buf, int nbytes)
{
	long size = 0, i, dInBlock[256], InBlock[256], midBlock = 0;
	OFT *oftp = running->fd[fd];
	int lbk = 0, startByte = 0, block = 0, remain = 0;
	MINODE *mip = running->fd[fd]->inodeptr;
	char wBuf[BLKSIZE], *cp, *cq;
	//create buffer of null
	for ( i = 0; i < BLKSIZE; i++)
		wBuf[i] = '\0';
	size = nbytes;
	//basically code given by KC
	//need to write indirect blocks and double indirect blocks
	while ( nbytes > 0 )
	{
		lbk = oftp->offset / BLKSIZE;
		startByte = oftp->offset % BLKSIZE;
		//direct blocks
		if ( lbk < 12 )
		{
			if ( mip->inode.i_block[lbk] == 0 )
				mip->inode.i_block[lbk] = balloc(dev);
			block = mip->inode.i_block[lbk];
		}
		//indirect blocks
		else if (lbk >= 12 && lbk < 12 + 256 )
		{
			//find block 12
			if ( mip->inode.i_block[12] == 0 )
			{
				mip->inode.i_block[12] = balloc(dev);
			}
			
			//get the block at 12			
			get_block(dev, mip->inode.i_block[12], dInBlock);
			//delete all associations
			if ( dInBlock[lbk - 12] == 0 )
			{
				dInBlock[lbk - 12] = balloc(dev);
				put_block(dev, mip->inode.i_block[12], dInBlock);
			}
			block = dInBlock[lbk - 12];
		}
		//double indriect blocks
		else
		{
				
			if ( mip->inode.i_block[13] == 0 )
			{
				mip->inode.i_block[13] = balloc(dev);
			}
			//get block 13
			get_block(dev, mip->inode.i_block[13], InBlock);
			if ( InBlock[(lbk - (12 + 256))/256] == 0 )
			{
				InBlock[(lbk - (12 + 256))/256] = balloc(dev);
				put_block(dev, mip->inode.i_block[13], InBlock);
			}
			midBlock = InBlock[(lbk - (12 + 256))/256];
			get_block(dev, midBlock, dInBlock);
			if ( dInBlock[(lbk - (12 + 256))%256] == 0 )
			{
				dInBlock[(lbk - (12 + 256))%256] = balloc(dev);
				put_block(dev, midBlock, dInBlock);
			}
			block = dInBlock[(lbk - (12 + 256))%256];
		}
		//read disk, cp points to start and number of bytes remain in block
		get_block(dev, block, wBuf);
		cp = wBuf + startByte;
		cq = buf;
		remain = BLKSIZE - startByte;
		while (remain > 0)
		{
			*cp++ = *cq++;            // cq points at buf[ ]       
			nbytes--;  
			remain--;
			oftp->offset++;
			if ( oftp->offset > mip->inode.i_size ) 
				mip->inode.i_size++;
			if (nbytes <= 0)
				break; 
		}
		put_block(dev, block, wBuf);
	}
	mip->dirty = 1;
	size -= nbytes;
	return size;
}

int cat_file(char* pathname)
{

	int fd = -1, printed = 0, tmp;
	long size, point = 0;
	//open file name for READ
	printf("---------------- Contents -----------------\n");
	printf("-------------------------------------------\n");
	fd = open_file(pathname, "R");
/*	if ( fd < 0 )
	{
		printf("CAT FAILED\n");
		return 0;
	}*/
	//get size of file
	size = running->fd[fd]->inodeptr->inode.i_size;
	//while printing
	while ( printed < size )
	{ 
		//get the next segment
		point = read_file(fd, 8192, 1);
		//update printed to next byte size
		printed += point;
	}
	printf("-------------------------------------------\n");	
	close_file(fd);
	return printed;
}
long lseek_file(int fd, long newPosition)
{
	long originalPosition = 0;	
	//if refCount < 0 break
	if ( running->fd[fd]->refCount > 0 )
	{
		//changeOFTs entry offset to position but make sure not to over run either end of file
		if ( newPosition < running->fd[fd]->inodeptr->inode.i_size && 	newPosition > -1 )
		{
			//retain original poisition
			originalPosition = running->fd[fd]->offset;
			if ( running->fd[fd]->refCount > 0 )
			{
				//set offset to zero
				running->fd[fd]->offset = 0;
			}
			//make offset newPosition
			running->fd[fd]->offset = newPosition;
		}
	}
	return originalPosition;
}
//////////////////////////////////// helpers /////////////////////
int CommandToke( char part1[], char part2[])
{
  char tokeMe[256];
  int i = 0, j = 0, flag = 0, k = 0, l = 0;

  printf("What would you like to do? ");
  gets(tokeMe);	//take in string
  while(tokeMe[i] != '\0')	//while string is not null
  {
	  if(flag == 0)	//to populate command string
	  {
			if(tokeMe[i] != ' ')	//if current letter is NOT a space
			  {
				part1[j] =  tokeMe[i]; //then copy character
				i++;	//increment indices for both strings
				j++;
			  }
			else
			  {
				part1[j] = '\0';	//if current letter is space then put a NULL character
				i++;	//increment tokeMe string
				j=0;	//reset copy string index
				flag = 1;	// next iteration, populate pathname string
			  }
	  }
	  else if(flag==1)	//to populate pathname string
	  {
				part2[j] =  tokeMe[i];	//then copy character
				i++;	//increment indices for both strings
				j++;
	  }
  }
  if(tokeMe[j] == '\0')	//if there was no pathname
  {
	  part1[j] = '\0';	//populate pathname with "$"
	  part2[0] = '$';
	  part2[1] = '\0';
	  k = 0;	//set length of pathname to 0
  }
  else
  {
	  //if there was a pathname
	  part2[j] = '\0';	//put NULL character at end of pathname string	
	  k=strlen(part2);	//calculate length of pathname
  }


  return k;
}
int linkToke(char part1[], char part2[])
{
  char tokeMe[256];
  int i = 0, j = 0, flag = 0, k = 0, l = 0;
  printf("What files would you like to link? ");
  gets(tokeMe);	//take in string
  while(tokeMe[i] != '\0')	//while string is not null
  {
	  if(flag == 0)	//to populate command string
	  {
			if(tokeMe[i] != ' ')	//if current letter is NOT a space
			  {
				part1[j] =  tokeMe[i]; //then copy character
				i++;	//increment indices for both strings
				j++;
			  }
			else
			  {
				part1[j] = '\0';	//if current letter is space then put a NULL character
				i++;	//increment tokeMe string
				j=0;	//reset copy string index
				flag = 1;	// next iteration, populate pathname string
			  }
	  }
	  else if(flag==1)	//to populate pathname string
	  {
				part2[j] =  tokeMe[i];	//then copy character
				i++;	//increment indices for both strings
				j++;

	  }
  }
  if(tokeMe[j] == '\0')	//if there was no pathname
  {
	  part1[j] = '\0';	//populate pathname with "$"
	  part2[0] = '$';
	  part2[1] = '\0';
	  k = 0;	//set length of pathname to 0
  }
  else
  {
	  //if there was a pathname
  }
  {
	  part2[j] = '\0';	//put NULL character at end of pathname string	
	  k=strlen(part2);	//calculate length of pathname
  }


  return k;
}
int chToke(char part1[], char part2[])
{
  char tokeMe[256];
  int i = 0, j = 0, flag = 0, k = 0, l = 0;
  printf("What would you like to do? ");
  gets(tokeMe);	//take in string
  while(tokeMe[i] != '\0')	//while string is not null
  {
	  if(flag == 0)	//to populate command string
	  {
			if(tokeMe[i] != ' ')	//if current letter is NOT a space
			  {
				part1[j] =  tokeMe[i]; //then copy character
				i++;	//increment indices for both strings
				j++;
			  }
			else
			  {
				part1[j] = '\0';	//if current letter is space then put a NULL character
				i++;	//increment tokeMe string
				j=0;	//reset copy string index
				flag = 1;	// next iteration, populate pathname string
			  }
	  }
	  else if(flag==1)	//to populate pathname string
	  {
				part2[j] =  tokeMe[i];	//then copy character
				i++;	//increment indices for both strings
				j++;

	  }
  }
  if(tokeMe[j] == '\0')	//if there was no pathname
  {
	  part1[j] = '\0';	//populate pathname with "$"
	  part2[0] = '$';
	  part2[1] = '\0';
	  k = 0;	//set length of pathname to 0
  }
  else
  {
	  //if there was a pathname
  }
  {
	  part2[j] = '\0';	//put NULL character at end of pathname string	
	  k=strlen(part2);	//calculate length of pathname
  }


  return k;
}
void PathToke(char pathname[], char part1[], char part2[], int length)
{
  int j = 0, i = 0, k = 0;

  j = length;	//copy the length of pathname
  for(k = 0; pathname[j] != '/'; k++, j--)	//counts the length of the basename in length
  {
  }
 
  while(i <= j)	//copies dirname until the last / is encountered
  {
	  part1[i] = pathname[i];
	  i++;
  }
  part1[i] = '\0';	//add NULL character
  
  length = 0;
  while(pathname[i] != '\0')	//copy the end of pathname for basename until NULL
  {
	  part2[length] = pathname[i];
	  length++;
	  i++;
  }
  part2[length] = '\0';	//add NULL character to end if basename
}
int decFreeInodes(int dev)
{
	char buf[BLKSIZE];
	lseek(dev, (long)BLKSIZE, 0);
	read(dev, buf, BLKSIZE);
	sp = (SUPER *)buf;
 	(sp->s_free_inodes_count)--;
 	lseek(dev, (long)BLKSIZE, 0);
	write(dev, buf, BLKSIZE);  
	lseek(dev, (long)(BLKSIZE * GDBLOCK), 0);
	read(dev, buf, BLKSIZE);
	gp = (GD *)buf;
	(gp->bg_free_inodes_count)--;
	lseek(dev, (long)(BLKSIZE * GDBLOCK), 0);
	write(dev, buf, BLKSIZE);
	return (gp->bg_free_inodes_count);
}
int incFreeInodes(int dev)
{
	char buf[BLKSIZE];
	lseek(dev, (long)BLKSIZE, 0);
	read(dev, buf, BLKSIZE);
	sp = (SUPER *)buf;
 	(sp->s_free_inodes_count)++;
 	lseek(dev, (long)BLKSIZE, 0);
	write(dev, buf, BLKSIZE);  
	lseek(dev, (long)(BLKSIZE * GDBLOCK), 0);
	read(dev, buf, BLKSIZE);
	gp = (GD *)buf;
	(gp->bg_free_inodes_count)++;
	lseek(dev, (long)(BLKSIZE * GDBLOCK), 0);
	write(dev, buf, BLKSIZE);
	return (gp->bg_free_inodes_count);
}
int decFreeBlocks(int dev)
{
	char buf[BLOCK_SIZE];
	lseek(dev, (long)BLOCK_SIZE, 0);
	read(dev, buf, BLOCK_SIZE);
	sp = (SUPER *)buf;
 	(sp->s_free_blocks_count)--;
 	lseek(dev, (long)BLOCK_SIZE, 0);
	write(dev, buf, BLOCK_SIZE);  
	lseek(dev, (long)(BLOCK_SIZE * GDBLOCK), 0);
	read(dev, buf, BLOCK_SIZE);
	gp = (GD *)buf;
	(gp->bg_free_blocks_count)--;
	lseek(dev, (long)(BLOCK_SIZE * GDBLOCK), 0);
	write(dev, buf, BLOCK_SIZE);
	return (gp->bg_free_blocks_count);
}
int incFreeBlocks(int dev)
{
	char buf[BLOCK_SIZE];
	lseek(dev, (long)BLOCK_SIZE, 0);
	read(dev, buf, BLOCK_SIZE);
	sp = (SUPER *)buf;
 	(sp->s_free_blocks_count)++;
 	lseek(dev, (long)BLOCK_SIZE, 0);
	write(dev, buf, BLOCK_SIZE);  
	lseek(dev, (long)(BLOCK_SIZE * GDBLOCK), 0);
	read(dev, buf, BLOCK_SIZE);
	gp = (GD *)buf;
	(gp->bg_free_blocks_count)++;
	lseek(dev, (long)(BLOCK_SIZE * GDBLOCK), 0);
	write(dev, buf, BLOCK_SIZE);
	return (gp->bg_free_blocks_count);
}
int tst_bit(char *buf, int i)
{ 	
	return buf[(i)/8] & (1 << (i%8));
}
int set_bit(char *buf, int i)
{		
	return buf[(i)/8] |= 1<<(i)%8;
}
int clear_bit(char *buf, int i)
{
	return buf[(i)/8] &= ~(1<<(i)%8);		
}
unsigned long ialloc(int dev)
{
	int i, j;
	char buf[BLKSIZE];
	int ninodes;
	//get super block
	char buf2[BLKSIZE];	
	get_block(dev, 1, buf2);
  	sp = (SUPER *)buf2;
	///////	
	ninodes = sp->s_inodes_per_group;
	get_block(dev, IBITMAP, buf); 
	for (i = 0; i < ninodes; i++)
	{	
		if (tst_bit(buf, i) == 0)
		{
			set_bit(buf, i);
			put_block(dev, IBITMAP, buf);   
			j = decFreeInodes(dev); 
			return i+1;
		}
	}
	return 0;                     // no more FREE inodes
} 
unsigned long idalloc(int dev, int clear)
{
	unsigned long i, j;
	char buf[BLKSIZE];
	get_block(dev, IBITMAP, buf);  
	if (tst_bit(buf, clear) == 1)
	{
		clear_bit(buf, clear);
		put_block(dev, IBITMAP, buf);   
		j = incFreeInodes(dev); 
		return clear;
	}
	return 0;                     // no more FREE inodes
} 
unsigned long balloc(int dev)
{
	unsigned long i, j;
	char buf[BLOCK_SIZE];   
	long hold[256];
	//get super block
	char buf2[BLKSIZE];	
	get_block(dev, 1, buf2);
  	sp = (SUPER *)buf2;
	///////	
	int binodes = sp->s_blocks_per_group;
	get_block(dev, BBITMAP, buf);       
	for (i = 0; i < binodes; i++)
	{	
		if (tst_bit(buf, i) == 0)
		{
			set_bit(buf, i);
			put_block(dev, BBITMAP, buf);
			get_block(dev, i, hold);
			for (j = 0; j < 256; j++)
				hold[j] = 0;
			put_block(dev, i, hold);   
			j = decFreeBlocks(dev);  
			return i;
		}
	}
	return 0;                     // no more FREE inodes
} 
unsigned bdalloc(int dev, int clear)
{
	unsigned long i, j;
	char buf[BLKSIZE];            
	get_block(dev, BBITMAP, buf);       
	if (tst_bit(buf, clear) == 1)
	{
		clear_bit(buf, clear);
		put_block(dev, BBITMAP, buf);   
		j = incFreeBlocks(dev);  
		return clear;
	}
	return 0;                     // no more FREE inodes
} 
int falloc(int check)
{	
	int i = 0;
	OFT *oftp = malloc(sizeof(OFT));
	if (check < 10)
	{
		oftp = running->fd[check];
		return oftp;
	}
	return 0;
}
int fdalloc(OFT *oftptr)
{	
	int i = 0;
	oftptr->refCount = 0;
	oftptr->inodeptr = 0;
	oftptr->offset = 0;
	oftptr->mode = -1;
	return 0;
}
int DTbdalloc(MINODE *mip)
{
	int i = 0, j = 0;
	unsigned long buf[BLKSIZE], inBuf[BLKSIZE]; 
	//search all inde blocks	
	if ( mip->inode.i_block[12] > 0 )
	{
		//search all indirect blocks
		get_block(dev, mip->inode.i_block[12], buf);
		for (i = 0; i < 256; i++)
		{
			//dealloc
			if ( buf[i] > 0 )
			{
				bdalloc(dev, buf[i]);
				buf[i] = 0;	
			}
		}
		//deallocate block 12
		bdalloc(dev, mip->inode.i_block[12]);
		put_block(dev, mip->inode.i_block[12], buf);
		mip->inode.i_block[12] = 0;		
		mip->dirty = 1;
	}
	//check double indirect blocks
	if ( mip->inode.i_block[13] > 0 )
	{
		get_block(dev, mip->inode.i_block[13], buf);
		for (i = 0; i < 256; i++)
		{
			if ( buf[i] > 0 )
			{
				get_block(dev, buf[i], inBuf);
				for (j = 0; j < 256; j++)
				{
					if ( inBuf[j] > 0 )
					{
						bdalloc(dev, inBuf[j]);
						inBuf[j] = 0;
					}
				}
				put_block(dev, buf[i], inBuf);
				bdalloc(dev, buf[i]);
				buf[i] = 0;
			}
		}
		bdalloc(dev, mip->inode.i_block[13]);
		put_block(dev, mip->inode.i_block[13], buf);
		mip->inode.i_block[13] = 0;		
		mip->dirty = 1;
	}
	return i;	
}
void menu(void)
{
	printf("-------------------------------------------\n");
	printf("---------------- MENU ---------------------\n");
	printf("Please choose commands from the list below:\n");
	printf("-------------------------------------------\n");
	printf("---------------- LEVEL 1 ------------------\n");
	printf("-------------------------------------------\n");	
	printf("menu:                    print menu options\n");
	printf("cd:                        change directory\n");
	printf("ls:                        list directories\n");
	printf("pwd: 		    print working directory\n");
	printf("mkdir: 			     make directory\n");
//	printf("rmdir: 			   remove directory\n");
	printf("creat: 				create file\n");
	printf("link:				  link file\n");
//	printf("unlink:(C) 			unlink file\n");
	printf("symlink:		 symbolic link file\n");
	printf("touch: 			   create new files\n");
	printf("chmod:                   change permissions\n");
	printf("open: 			       opens a file\n");
	//printf("close:(C) 		      closes a file\n");
	printf("pfd: 		       print file directory\n");
	printf("read: 		           read from a file\n");
   // printf("write:(C)	            write to a file\n");
	printf("cat: 	       write out contents of a file\n");
	printf("lseek: 	    reposition to read/write offset\n");
	printf("quit: 			       quit program\n");
	printf("-------------------------------------------\n");

}
