/* 
 * Name:    Li Ce (Shawn) Wang
 * ID:      v00878878 
 */
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
#include "vdisk.h"   



void print_bitmap(bitmap_t* bitmap){
    int i = 0; 
    for (i = 0; i < sizeof(bitmap->bitmap); i++){
        printf("Allocated Byte [%d]", i);
        printf(": "BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(bitmap->bitmap[i]));
    }
}

bitmap_t* set_bitmap (bitmap_t* bitmap, int idx, int status){
    uint16_t index = 0;
    int bit_index = 0, val = 0;
    switch (status){
        case ALLOCATE: 
            index = idx / 8; 
            bit_index = idx % 8;          
            val = (uint16_t) pow(2, bit_index); 
            bitmap->bitmap[index] = bitmap->bitmap[index] & ~(val); 
            break; 
        case DEALLOCATE: 
            index = idx / 8; 
            bit_index = idx % 8;          
            val = (int) pow(2, bit_index); 
            bitmap->bitmap[index] = bitmap->bitmap[index] | (val);
            break; 
        default: 
            break; 
    }
    return bitmap;
}

int find_firstfreeblock (bitmap_t* bitmap){
    bitmap_t* opp_bitmap = NULL; 
    int index = 0; 
    int bit_index = 0;
    int found = 0; 
    int val = 0; 
    
    opp_bitmap = calloc(1, BLOCK_SIZE); 
    
    while (index < BLOCKS_PER_DISK/8) {
        bit_index = 0; 
        while(bit_index < 8){
            val = (int)pow(2, bit_index); 
            opp_bitmap->bitmap[index] = opp_bitmap->bitmap[index] | (val);
            if ((bitmap->bitmap[index]) & (opp_bitmap->bitmap[index])){
                found = 1; 
                break;
            }
            bit_index ++; 
        }
        if (!found)
            index++;  
        else 
            break;  
    }
    
    free(opp_bitmap);    
    if (found){
        return index * 8 + bit_index; 
    }
    
    printf("LLFS ERROR: Free block not found on vdisk\n");
    return -1; 
}

inode_t* read_imap(uint16_t inode_num){
    int cr_imap_blckidx = 0; 
    int cr_imap_idx = 0; 
    uint16_t ret = 0; 
    inode_t* inode = NULL;
    
    inode = calloc(1, BLOCK_SIZE);
    
    if ((inode_num % IMAPBLKS_PER_DISK) == 0) {
        cr_imap_blckidx = inode_num/IMAPBLKS_PER_DISK; 
    }
    else{
        cr_imap_blckidx = inode_num/IMAPBLKS_PER_DISK;
        cr_imap_idx = inode_num % IMAPBLKS_PER_DISK; 
    }
    if(inode_num < BLOCKS_PER_DISK){
        printf("Find inode number %"PRIu16" on cr_imap\n", inode_num); 
        printf("Updating checkpoint region cr_imap block[%d]: block imap[%d]\n", cr_imap_blckidx, cr_imap_idx); 
        ret = cr_imap->cr_imap[cr_imap_blckidx].imap[cr_imap_idx]; 
        lseek(superblock->disk_fd, ret * BLOCK_SIZE, SEEK_SET);
        read(superblock->disk_fd, inode, BLOCK_SIZE);
    }
    else{
        perror("LLFS ERROR: Checkpoint region read error\n");
        return -1; 
    }
    return inode;
}

cr_t* write_imap(cr_t* cr_imap, uint16_t inode_num){
    int cr_imap_blckidx = 0; 
    int cr_imap_idx = 0; 
    
    if ((inode_num % IMAPBLKS_PER_DISK) == 0) {
        cr_imap_blckidx = inode_num/IMAPBLKS_PER_DISK; 
    }
    else{
        cr_imap_blckidx = inode_num/IMAPBLKS_PER_DISK;
        cr_imap_idx = inode_num % IMAPBLKS_PER_DISK; 
    }
    if(inode_num < BLOCKS_PER_DISK){
        printf("Find inode number %"PRIu16" on cr_imap\n", inode_num); 
        printf("Updating checkpoint region cr_imap block[%d]: block imap[%d]\n", cr_imap_blckidx, cr_imap_idx);
        cr_imap->cr_imap[cr_imap_blckidx].imap[cr_imap_idx] = inode_num; 
        lseek(superblock->disk_fd, CR_IMAP_OFFSET, SEEK_SET);
        write(superblock->disk_fd, cr_imap, BLOCK_SIZE * CRBLKS_PER_DISK);
    }
    else{
        perror("LLFS ERROR: Checkpoint region write error\n");
        return -1; 
    }
    return cr_imap;
}

// Inode links count 
int read_inode(inode_t* inode, uint16_t index_limit){
    int index = 0;
    int found = 0; 
       
    // direct blocks
    if (index_limit <= N_DIRECT){
        while(index <= index_limit){
            if (inode->dir_blocks[index] == 0){             
                found = 1;
                break;  
            }
            index++; 
        }
    }
    if (found){
        printf("Inode limit diskmap[%d]\n", index); 
        return index; 
    }
    perror("LLFS ERROR: Read inode diskmap error\n");
    return -1;        
}

// block directory count
uint16_t read_dir(inode_t* inode, uint16_t index_limit){ // index limit -> link count
    block_t* dir_block; 
    int inode_idx = 0, dir_idx = 0;  
    int found = 0;  
    
    // direct blocks
    if (index_limit <= N_DIRECT){
        while(inode_idx < index_limit){
            dir_block = calloc (1, BLOCK_SIZE);
            dir_block = read_block(superblock->disk_fd, dir_block, inode->dir_blocks[inode_idx]); 
            while (dir_idx < DIRS_PER_BLOCK){
                if (dir_block->dir_list[dir_idx].component_val[0] == NULL){
                    found = 1; 
                    break; 
                }
                dir_idx++;    
            }
            if (found){
                printf("Directory [%d] found available in current directory level\n", inode_idx * DIRS_PER_BLOCK + dir_idx);
                return inode_idx * DIRS_PER_BLOCK + dir_idx;   
            }
            else {
                dir_idx = 0; 
                inode_idx++;
            }
        }
    }
    if (found){
        printf("Directory [%d] available in current directory level\n", inode_idx * DIRS_PER_BLOCK + dir_idx);
        return inode_idx*DIRS_PER_BLOCK + dir_idx;   
    }
    printf("LLFS ERROR: Read current directory list error\n");
    return -1;   
}

// block directory count
uint16_t find_dir(inode_t* inode, char* path_name, int index_limit){ // index limit -> link count
    block_t* dir_block = NULL; 
    int inode_idx = 0; 
    int dir_idx = 0;   

    // direct blocks
    if (index_limit <= N_DIRECT){
        while(inode_idx < index_limit){
            dir_block = calloc (1, BLOCK_SIZE);
            dir_block = read_block(superblock->disk_fd, dir_block, inode->dir_blocks[inode_idx]); 
            while(dir_block->dir_list[dir_idx].component_val[0] != NULL){
                printf("%s: path name found in current directory level\n", dir_block->dir_list[dir_idx].component_val);
                if (!(strcmp(dir_block->dir_list[dir_idx].component_val, path_name))){
                    return dir_block->dir_list[dir_idx].inode_num;
                }
                dir_idx++; 
            }
            dir_idx = 0; 
            inode_idx++;
        }
        if (inode_idx >= index_limit){
            printf("%s: path name not found in current directory level\n", path_name);
            return 0;   
        }
    }
    printf("LLFS ERROR: Find directory error\n");
    return -1;        
}

block_t* read_block(uint32_t disk_fd, block_t* block, uint16_t block_id){    
    
    if(block_id < BLOCKS_PER_DISK){
        printf("Reading disk byte offset = %llu: block size = %llu\n", (block_id) * BLOCK_SIZE, BLOCK_SIZE);
        lseek(disk_fd, (block_id) * BLOCK_SIZE, SEEK_SET);
        read(disk_fd, block, BLOCK_SIZE);
    }
    else{
        perror("LLFS ERROR: Read block error\n");
        return -1; 
    } 
    return block; 
}

// block count inode count & segment_t* & malloc block 
segment_t* read_segment (segment_t* segment, inode_t* inode, uint8_t file_type, uint16_t inode_num){
    int file_size = 0; 
    int blocks_needed = 0; 
    int i = 0; 
    
    switch (file_type){
        case DIRECTORY: 
            file_size = inode->data_size; 
            if (file_size % BLOCK_SIZE == 0) {
                blocks_needed = file_size/BLOCK_SIZE; 
            }
            else{
                blocks_needed = file_size/BLOCK_SIZE + 1;
            }
            for (i = 0; i < blocks_needed; i++){
                read_block(superblock->disk_fd, segment->blocks->dir_list, inode->dir_blocks[i]); 
            }              
            break; 
        case DATA:
            blocks_needed = inode->link_count;
            for (i = 0; i < blocks_needed; i++){
                read_block(superblock->disk_fd, segment->blocks + i, inode->dir_blocks[0] + i); 
            }   
            break; 
        default:
            break;
    }
    return segment; 
}

int write_block(uint32_t disk_fd, block_t* block, uint16_t block_id){
    int ret = 0;
    
    if(block_id < BLOCKS_PER_DISK){
        printf("Writing disk byte offset = %llu: block size = %llu\n", (block_id) * BLOCK_SIZE, BLOCK_SIZE);
        lseek(disk_fd, (block_id) * BLOCK_SIZE, SEEK_SET);
        ret = write(disk_fd, block, BLOCK_SIZE);
    }
    else{
        printf("LLFS ERROR: Write block error\n");
        return -1; 
    }
    return ret;
}

// block count inode count & segment_t* & malloc block 
void write_segment (segment_t* segment, inode_t* inode, uint8_t file_type){    
    int file_size = 0; 
    int blocks_needed = 0; 
    int i = 0; 
    
    // write data blocks
    switch (file_type){
        case DIRECTORY: 
            file_size = inode->data_size; 
            if (file_size % BLOCK_SIZE == 0) {
                blocks_needed = file_size/BLOCK_SIZE;
            }
            else{
                blocks_needed = file_size/BLOCK_SIZE + 1;
            }
            for (i = 0; i < blocks_needed; i++){
                printf("bitmap_data: %d\n", find_firstfreeblock(bitmap));
                write_block(superblock->disk_fd, segment->blocks + i, find_firstfreeblock(bitmap)); 
                // update bitmap block
                bitmap = set_bitmap(bitmap, find_firstfreeblock(bitmap), ALLOCATE);
                write_block(superblock->disk_fd, bitmap, 1); 
            }
            // update superblock 
            superblock->diskblocks_num += i;
            write_block(superblock->disk_fd, superblock, 0); 
            break;
        case DATA:
            blocks_needed = inode->link_count; 
            file_size = blocks_needed * BLOCK_SIZE; 
            for (i = 0; i < blocks_needed; i++){
                printf("bitmap_data: %d\n", find_firstfreeblock(bitmap));
                write_block(superblock->disk_fd, segment->blocks + i, find_firstfreeblock(bitmap)); 
                // update bitmap block
                bitmap = set_bitmap(bitmap, find_firstfreeblock(bitmap), ALLOCATE);
                write_block(superblock->disk_fd, bitmap, 1); 
            }
            // update superblock 
            superblock->diskblocks_num += i;
            write_block(superblock->disk_fd, superblock, 0); 
            break; 
        default:
            break;
    }
    // write inode blocks
    printf("bitmap_data: %d\n", find_firstfreeblock(bitmap));
    inode->inode_num = (uint16_t) find_firstfreeblock(bitmap); 
    write_block(superblock->disk_fd, inode, find_firstfreeblock(bitmap));
    // update bitmap block 
    bitmap = set_bitmap(bitmap, find_firstfreeblock(bitmap), ALLOCATE); 
    write_block(superblock->disk_fd, bitmap, 1);
    // update superblock 
    superblock->diskblocks_num += 1; 
    superblock->diskinodes_num += 1;  
    write_block(superblock->disk_fd, superblock, 0); 
    printf("superblock blocks count: %"PRIu32"\n", superblock->diskblocks_num); 
    printf("superblock inodes count: %"PRIu32"\n", superblock->diskinodes_num); 
}
