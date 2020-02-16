/* 
 * Name:    Li Ce (Shawn) Wang
 * ID:      v00878878 
 */
#ifndef VDISK_H
#define VDISK_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>   // mmap function lib
#include <fcntl.h>      // basic file control functions lib
#include <unistd.h>     // miscellaneous functions lib
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <time.h> 
#include <math.h>
#include <assert.h>
#include "File.h"

#define DISK_SIZE           2097152
#define BLOCK_SIZE          512     // block size 512 bytes
#define DIR_SIZE            32
#define BLOCKS_PER_DISK     4096

#define DIRS_PER_BLOCK      16
#define PTRS_PER_BLOCK      256
#define INODENUM_PER_BLOCK  256
#define IMAPBLKS_PER_DISK   16
#define CRBLKS_PER_DISK     17
#define BLOCKS_PER_SEGMENT  250
#define SEGMENTS_PER_DISK   60

#define SUPERBLOCK_OFFSET   0       // x00000000
#define BITMAP_OFFSET       512     // x00000200
#define CR_META_OFFSET      1024
#define CR_IMAP_OFFSET      1536

#define DIRECTORY   0
#define DATA        1
#define ALLOCATE    0
#define DEALLOCATE  1

#define N_DIRECT    250

#define MAX_FILENAME    31    
#define MAX_LINESIZE    512
#define MAX_FILESIZE    128000     // largest possible byte value excluding LLFS reserved blocks & root blocks

#define ROOT "/"

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c\n"
#define BYTE_TO_BINARY(byte)  \
  (byte & 0x80 ? '1' : '0'), \
  (byte & 0x40 ? '1' : '0'), \
  (byte & 0x20 ? '1' : '0'), \
  (byte & 0x10 ? '1' : '0'), \
  (byte & 0x08 ? '1' : '0'), \
  (byte & 0x04 ? '1' : '0'), \
  (byte & 0x02 ? '1' : '0'), \
  (byte & 0x01 ? '1' : '0') 

typedef struct superblock{
    uint32_t disk_fd;                       // magic number
    uint32_t diskblocks_num; 
    uint32_t diskinodes_num; 
}superblock_t; 

typedef struct bitmap{
    uint8_t bitmap[BLOCKS_PER_DISK/8]; 
} bitmap_t;

typedef struct cr_metadata{
    uint16_t seg_hdr;                       // block number
    uint16_t seg_tail;                      // block number
    uint16_t live_block;            		// number of live blocks
    time_t time_stamp; 
}cr_metadata_t;

typedef struct seg_summary{
    uint16_t seg_hdr; 
    uint16_t seg_tail;
    uint16_t live_block;            // number of live blocks
    time_t time_stamp;              // most recent modified of any block in segment
}seg_metadata_t;

typedef struct imap{
    uint16_t imap[INODENUM_PER_BLOCK]; 		// 256 inode numbers 
} imap_t;

typedef struct checkpoint_region{
    //cr_metadata_t cr_metadata; 
    imap_t cr_imap[IMAPBLKS_PER_DISK];      // 16 imap blocks 
} cr_t;

typedef struct dir_entry{
    unsigned char component_val[30]; 
    uint16_t inode_num;                     // block number
} dir_t;

typedef struct s_indir_block{
    uint16_t indir_block[PTRS_PER_BLOCK];   // 64 pointers or 512 bytes
} s_indir_t;

typedef struct d_indir_block{
    uint16_t indir_block[PTRS_PER_BLOCK]; 	// 64 pointers or 512 bytes
} d_indir_t;

typedef struct inode{
    uint32_t data_size;             // 4 bytes
    uint16_t inode_num;             // 2 bytes
    uint16_t link_count;            // 2 bytes
    uint8_t file_type;              // 1 byte 
    uint16_t dir_blocks[N_DIRECT];  // 500 bytes
} inode_t;

// A block used as struct converter for write() or read()
typedef union block{      //union: all members are stored at the same address & accessed either or one at a time
    superblock_t superblock; 
    bitmap_t bitmap;
    cr_metadata_t cr_metadata; 
    seg_metadata_t seg_summary; 
    unsigned char data[BLOCK_SIZE]; 
    dir_t dir_list[DIRS_PER_BLOCK];
    inode_t inode;
    imap_t imap; 
}block_t;

typedef struct segment{
    block_t blocks[BLOCKS_PER_SEGMENT]; 
} segment_t;

typedef struct disk{
    superblock_t superblock; 
    bitmap_t bitmap; 
    cr_metadata_t cr_meta; 
    imap_t cr_imap[IMAPBLKS_PER_DISK]; 
    block_t log[BLOCKS_PER_DISK - IMAPBLKS_PER_DISK - 3]; 
} disk_t;

// cache memory
disk_t* vdisk;
superblock_t* superblock; 
bitmap_t* bitmap; 
cr_metadata_t* cr_meta; 
cr_t* cr_imap; 
imap_t* imap_block;

// vdisk.c function headers 
void print_bitmap(bitmap_t* bitmap);
bitmap_t* set_bitmap (bitmap_t* bitmap, int idx, int status);
int find_firstfreeblock (bitmap_t* bitmap);
int read_inode(inode_t* inode, uint16_t index_limit);
uint16_t read_dir(inode_t* inode, uint16_t index_limit);
uint16_t find_dir(inode_t* inode, char* path_name, int index_limit);
block_t* read_block(uint32_t disk_fd, block_t* block, uint16_t block_id);
segment_t* read_segment (segment_t* segment, inode_t* inode, uint8_t file_type, uint16_t inode_num);
int write_block(uint32_t disk_fd, block_t* block, uint16_t block_id);
void write_segment (segment_t* segment, inode_t* inode, uint8_t file_type);

#endif	
