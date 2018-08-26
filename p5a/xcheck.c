#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <sys/mman.h>
#include <string.h>
#include "fs.h"

#define T_DIR 1
#define T_FILE 2                                                                       
#define T_DEV 3
// Errors to be printed to stderror
#define IMAGE_NOT_FOUND "image not found."
#define INODE_CHECK "ERROR: bad inode."
#define BAD_DADDRESS "ERROR: bad direct address in inode."
#define BAD_IADDRESS "ERROR: bad indirect address in inode."
#define ROOT_DIR "ERROR: root directory does not exist."
#define DIR_FORMAT "ERROR: directory not properly formatted."
#define ADDR_FREE "ERROR: address used by inode but marked free in bitmap."
#define BMP_FREE "ERROR: bitmap marks block in use but it is not in use."
#define DADDRESS "ERROR: direct address used more than once."
#define IADDRESS "ERROR: indirect address used more than once."
#define INODE_DIR "ERROR: inode marked use but not found in a directory."
#define FREE_INODE "ERROR: inode referred to in directory but marked free."
#define FILE_REF "ERROR: bad reference count for file."
#define DIR_DUP "ERROR: directory appears more than once in file system."


// Globals
struct superblock *sb;
struct dinode *dinp;
void *map;
char *bitmap;

//function declarations
void checkAddress(char *bmp, int addr, int offst);
int get_addr(int offst, struct dinode *currdinode, int indirect);
int iterateBlocks(int *inodes, char *bmp, int inum, int parent_inum);
void runChecks(struct dinode *p, int in);
int init_sb(int fd);
int setup_inodes(int pos);
char offsetted(int pos);

int main(int argc, char **argv)
{
  //open file system image
  int fd = open(argv[1], O_RDONLY);
  if (fd < 0)
  {
    fprintf(stderr, "%s\n", IMAGE_NOT_FOUND);
    exit(1);
  }

  //initialize and map file system
  int pos = init_sb(fd);

  //initialize inodes
  int inodes[sb->ninodes + 1];
  memset(inodes, 0, (sb->ninodes + 1) * sizeof(int));
  size_t bmp_sz = setup_inodes(pos);
  int i;

  //check for root dir
  if (!(dinp + ROOTINO) || (dinp + ROOTINO)->type != T_DIR)
  {
    fprintf(stderr, "%s\n", ROOT_DIR);
    exit(1);
  }

  //initialize bitmap
  char bmp[bmp_sz];
  memset(bmp, 0, bmp_sz);
  memset(bmp, 0xFF, pos / 8);
  bmp[pos / 8] = offsetted(pos);

  //check blocks
  iterateBlocks(inodes, bmp, ROOTINO, ROOTINO);
  struct dinode *currdinode = dinp;

  currdinode = dinp;
  //check for incorrect bmp
  i = 1;
  while (i < bmp_sz + 1)
  {
    if (bitmap[i] != bmp[i])
    {
      fprintf(stderr, "%s\n", BMP_FREE);
      exit(1);
    }
    i++;
  }

  //run basic inode checks
  for (i = 1, ++currdinode; i < sb->ninodes; ++i, ++currdinode)
  {
    if (currdinode->type != 0)
      runChecks(currdinode, inodes[i]);
  }
  exit(0);
}

void checkAddress(char *bmp, int addr, int offst)
{
  if (addr == 0)
    return;

  // Block number out of bound
  if (addr < (sb->ninodes / IPB + sb->nblocks / BPB + 4) || addr >= (sb->ninodes / IPB + sb->nblocks / BPB + 4 + sb->nblocks))
  {
    if (!((offst / BSIZE) < NDIRECT))
      fprintf(stderr, "%s\n", BAD_IADDRESS);
    else
      fprintf(stderr, "%s\n", BAD_DADDRESS);
    exit(1);
  }

  // In use but marked free
  char byte = *(bitmap + addr / 8);
  if (!((byte >> (addr % 8)) & 1))
  {
    fprintf(stderr, "%s\n", ADDR_FREE);
    exit(1);
  }
}

int get_addr(int offst, struct dinode *currdinode, int indirect)
{
  return (offst / BSIZE <= NDIRECT && !indirect) ?
        currdinode->addrs[offst / BSIZE] :
        *((int *)(map + currdinode->addrs[NDIRECT] * BSIZE) + offst / BSIZE - NDIRECT);
}

int iterateBlocks(int *inodes, char *bmp, int inum, int parent_inum)
{
  struct dinode *currdinode = dinp + inum;

  //bad directory reference
  inodes[inum]++;
  if (inodes[inum] > 1 && currdinode->type == T_DIR)
  {
    fprintf(stderr, "%s\n", DIR_DUP);
    exit(1);
  }

  //bad directory format
  if (currdinode->type == T_DIR && currdinode->size == 0)
  {
    fprintf(stderr, "%s\n", DIR_FORMAT);
  }

  //invalid directory
  if (currdinode->type == 0)
  {
    fprintf(stderr, "%s\n", FREE_INODE);
    exit(1);
  }

  int offst = -1;
  int indirect = 0;

  for (offst = 0; offst < currdinode->size; offst += BSIZE)
  {
    int blk_num = get_addr(offst, currdinode, indirect);

    checkAddress(bmp, blk_num, offst);

    if (offst / BSIZE == NDIRECT && !indirect)
    {
      offst -= BSIZE;
      indirect = 1;
    }

    //check indirect and direct pointers
    if (inodes[inum] == 1)
    {
      char byte = *(bmp + blk_num / 8);
      if (!((byte >> (blk_num % 8)) & 1))
      {
        byte = byte | (1 << (blk_num % 8));
        *(bmp + blk_num / 8) = byte;
      }
      else
      {
        if (indirect)
          fprintf(stderr, "%s\n", IADDRESS);
        else
          fprintf(stderr, "%s\n", DADDRESS);
        exit(1);
      }
    }

    if (currdinode->type == T_DIR)
    {
      struct dirent *dent;
      if (!indirect)
        dent = (struct dirent *)(map + blk_num * BSIZE);

      if (0 == offst)
      {
        if (strcmp(dent->name, ".") || strcmp((dent + 1)->name, ".."))
        {
          fprintf(stderr, "%s\n", DIR_FORMAT);
          exit(1);
        }
        dent += 2;
        //must have root directory
        if ((dent - 1)->inum != parent_inum)
        {
          fprintf(stderr, "%s\n", ROOT_DIR);
          exit(1);
        }

        
      }

      //iterate through total number of directories
      while (dent < (struct dirent *)(ulong)(map + (blk_num + 1) * BSIZE))
      {
        if (dent->inum != 0)
          iterateBlocks(inodes, bmp, dent->inum, inum);
        dent++;
      }
    }
  }
  return 0;
}

void runChecks(struct dinode *p, int in)
{
  //invalid types
  if (p->type != T_FILE)
    if (p->type != T_DIR)
      if (p->type != T_DEV)
      {
        fprintf(stderr, "%s\n", INODE_CHECK);
        exit(1);
      }

  //inode in use but not in directory
  if (in == 0)
  {
    fprintf(stderr, "%s\n", INODE_DIR);
    exit(1);
  }

  //bad reference count
  if (p != &dinp[ROOTINO])
    if (in != p->nlink)
    {
      fprintf(stderr, "%s\n", FILE_REF);
      exit(1);
    }

  //extra links on directories
  if (p->type == T_DIR)
    if (in > 1)
    {
      fprintf(stderr, "%s\n", DIR_DUP);
      exit(1);
    }
}

int init_sb(int fd)
{
  struct stat sbuf;    
  fstat(fd, &sbuf);

  // mmap
  map = mmap(NULL, sbuf.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
  if (!map)
    printf("Failed to map fs image.\n");

  sb = (struct superblock *)(map + BSIZE);
  return sb->ninodes / IPB + sb->nblocks / BPB + 4;
}

int setup_inodes(int pos)
{
  dinp = (struct dinode *)(map + 2 * BSIZE);
  bitmap = (char *)(map + (sb->ninodes / IPB + 3) * BSIZE);
  return (sb->nblocks + pos) / 8;
}

char offsetted(int pos)
{
  char last = 0;
  int mod_offset = pos % 8;
  int i = 0;
  while (i++ < mod_offset)
  {
    last = (last << 1) | 1;
  }
  return last;
}
