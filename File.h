/* 
 * Name:    Li Ce (Shawn) Wang
 * ID:      v00878878 
 */
#ifndef DEFINE_H
#define DEFINE_H

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
#include "vdisk.h"      // vdisk header file 



// File.c funtion headers
void lfs_initialize (char *disk_name); 
inode_t* lfs_mkroot(void);
inode_t* lfs_create_inode(inode_t* inode, size_t file_type); 
dir_t* lfs_dir_entry(dir_t* dir, const char* component_name); 
inode_t* lfs_mkdir(inode_t* curr, char* path); 
char* lfs_read_file(inode_t* curr, const char* path);
inode_t* lfs_write_file(inode_t* curr, char* buffer, char* path);

#endif	