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


// LFS initialization 
void lfs_initialize (char *disk_name){
    int i = 0;  
    int fd = open(disk_name, O_CREAT | O_RDWR, 0777); // |O_PATH

    // Create LLFS vdisk  
    lseek(fd, 0, SEEK_SET);
    for (i = 0; i < DISK_SIZE; i++){
        if(write(fd, "", 1) != 1){
            perror("LLFS vdisk initialization error");
        } 
    } 
    lseek(fd, 0, SEEK_SET); 
    int disksize = lseek(fd, 0, SEEK_END); // 00200000 
    lseek(fd, 0, SEEK_SET); 
    printf("Created vdisk size: %d bytes\n", disksize); 
    
    // Initialize superblock - block 0
    superblock = calloc (1, BLOCK_SIZE); 
    superblock->disk_fd =  fd;
    printf("superblock fd: %"PRIu32"\n", superblock->disk_fd); 
    superblock->diskblocks_num = 1; 
    printf("superblock blocks count: %"PRIu32"\n", superblock->diskblocks_num); 
    superblock->diskinodes_num = 0;    
    printf("superblock inodes count: %"PRIu32"\n", superblock->diskinodes_num); 
    lseek(fd, SUPERBLOCK_OFFSET, SEEK_SET);
    write(fd, superblock, BLOCK_SIZE); 
    printf("superblock block size: %ld bytes\n", sizeof(vdisk->superblock)); 


    // Initialize bitmap block - block 1 (set block 0 - 9 bit unavailable)
    bitmap = calloc(1, BLOCK_SIZE);   
    for (i = 0; i < BLOCKS_PER_DISK; i++){
        bitmap = set_bitmap(bitmap, i, DEALLOCATE);
    }   
    for (i = 0; i < 2; i++){
        bitmap = set_bitmap(bitmap, i, ALLOCATE);
    }
    lseek(fd, BITMAP_OFFSET, SEEK_SET);
    write(fd, bitmap, BLOCK_SIZE);
    //print_bitmap(bitmap);
          
    // Initialize checkpoint region metadata block - block 2
    cr_meta = calloc(1, BLOCK_SIZE);
    cr_meta->seg_hdr = 0; 
    cr_meta->seg_tail =  0; 
    cr_meta->time_stamp = time(NULL);  
    printf("Timestamp: %ld\n" ,cr_meta->time_stamp);
    printf("CR metadata block size: %d bytes\n", sizeof(vdisk->cr_meta));
    lseek(fd, CR_META_OFFSET, SEEK_SET);
    write(fd, cr_meta, BLOCK_SIZE); 
    
    // Initialize checkpoint region imap blocks - block 3 - 18
    cr_imap = calloc(1, BLOCK_SIZE * IMAPBLKS_PER_DISK); 
    printf("CR imap block size: %d bytes\n", sizeof(vdisk->cr_imap));
    lseek(fd, CR_IMAP_OFFSET, SEEK_SET);
    write(fd, cr_imap, BLOCK_SIZE * IMAPBLKS_PER_DISK); 
    // Update checkpoint region bitmap 
    for (i = 2; i < CRBLKS_PER_DISK + 2; i++){
        bitmap = set_bitmap(bitmap, i, ALLOCATE);  
    }
    lseek(fd, BITMAP_OFFSET, SEEK_SET);
    write(fd, bitmap, BLOCK_SIZE);
    print_bitmap(bitmap);
    
    // Update superblock disk block number 
    superblock->diskblocks_num = i;     // diskblocks_num = (i)17 + 1 
    printf("superblock blocks count: %"PRIu32"\n", superblock->diskblocks_num); 
    printf("superblock inodes count: %"PRIu32"\n", superblock->diskinodes_num); 
    lseek(fd, SUPERBLOCK_OFFSET, SEEK_SET);
    write(fd, superblock, BLOCK_SIZE);    
}


inode_t* lfs_mkroot(void){
    block_t* root_data = NULL;
    inode_t* root_inode = NULL;
    
    // Initialize root data block 
    root_data = calloc (1, BLOCK_SIZE);  
    lfs_dir_entry(root_data->dir_list, "."); 
    lfs_dir_entry(root_data->dir_list, "..");
    printf("self . inode_num: %"PRIu16"\n", root_data->dir_list[0].inode_num); 
    printf("parent .. inode_num: %"PRIu16"\n", root_data->dir_list[1].inode_num); 
    
    // Initialize directory inode block 
    root_inode = lfs_create_inode(root_inode, DIRECTORY); 
    root_inode->data_size += (uint32_t) DIR_SIZE * 2; 
    root_inode->link_count += 1;
    root_inode->dir_blocks[0] = root_data->dir_list[0].inode_num;
    
    // Write root directory to disk
    write_segment(root_data, root_inode, root_inode->file_type); 
    print_bitmap(bitmap);
    
    free(root_data);
    root_data = NULL; 

    return root_inode; 
}

inode_t* lfs_create_inode(inode_t* inode, size_t file_type){
    inode_t* inode_block; 
    int i; 
    inode = calloc (1, BLOCK_SIZE);
          
    inode->data_size = 0; //BLOCK_SIZE
    inode->inode_num = (uint16_t) find_firstfreeblock(bitmap); 
    printf("inode_num: %"PRIu16"\n", inode->inode_num);
    inode->link_count = 0; 
    inode->file_type = (uint8_t) file_type;  

    for (i = 0; i < sizeof(inode->dir_blocks)/sizeof(inode->dir_blocks[0]); i++){
        inode->dir_blocks[i] = 0;
    }
    return inode; 
}

dir_t* lfs_dir_entry(dir_t* dir, const char* component_name){  
    int i = 0;       
    
    while (dir[i].component_val[0] != '\0'){
        printf("component_val: %s\n", dir[i].component_val);
        i++; 
    }
    if (i < DIRS_PER_BLOCK){
        strcpy(dir[i].component_val, component_name);
        printf("component_val: %s\n", dir[i].component_val);
        dir[i].inode_num = (uint16_t) find_firstfreeblock(bitmap);  	
        return dir; 
    }
    return 0; 
}

inode_t* lfs_mkdir(inode_t* curr, char* path){
    segment_t* slf_dir_data = NULL;
    segment_t* prt_dir_data = NULL; 
    inode_t* slf_dir_inode = NULL; 
    inode_t* prt_dir_inode = NULL;
    char* path_name = NULL; 
    int path_length = 0; 
    int index = 0; 
    int i = 0; 
    
    prt_dir_inode = calloc (1, BLOCK_SIZE);
    read_block(superblock->disk_fd, prt_dir_inode, curr->inode_num); 
    
    //Directory path error checking
    if (path == NULL){
        perror("LLFS invalid %s path name error\n"); 
    }
    path_length = strlen(path) + 1; // up to but not include '\0'
    path_name = (char*) calloc (1, sizeof(char) * (path_length));  
    strcpy (path_name, path); 
    
    if (path_length > MAX_FILENAME){
        perror("LLFS invalid %s path name exceeding 30 characters limit error\n"); 
    }
    for (i = 0; i < path_length; i++){
        if (!strcmp(&path_name[i], "/")){
            perror("LLFS invalid %s path name error\n"); 
        }
    }
    // Existing directory checking 
    if (find_dir(curr, path_name, curr->link_count)){
        printf("LLFS directory name found to exist\n"); 
        printf("LLFS require removing existing directory and associated child directories before creating the same directory\n");
        return NULL; 
    }  
    if (prt_dir_inode->link_count > N_DIRECT){
        printf("LLFS error: exceeded the current directory file limit > 128,000 bytes\n");
        return NULL; 
    }
    
    // Initialize directory data block 
    slf_dir_data = calloc(1, BLOCK_SIZE * BLOCKS_PER_SEGMENT);  
    lfs_dir_entry(slf_dir_data->blocks->dir_list, ".");        
    lfs_dir_entry(slf_dir_data->blocks->dir_list, "..");
    slf_dir_data->blocks->dir_list[1].inode_num = prt_dir_inode->inode_num; 
    printf("self . inode_num: %"PRIu16"\n", slf_dir_data->blocks->dir_list[0].inode_num); 
    printf("parent .. inode_num: %"PRIu16"\n", slf_dir_data->blocks->dir_list[1].inode_num); 
    
    // Initialize directory inode block 
    slf_dir_inode = lfs_create_inode(slf_dir_inode, DIRECTORY); 
    slf_dir_inode->data_size += (uint32_t) DIR_SIZE * 2; 
    slf_dir_inode->link_count += 1; 
    slf_dir_inode->dir_blocks[0] = slf_dir_data->blocks->dir_list[0].inode_num;
    printf("dir_data: %"PRIu16"\n", slf_dir_inode->dir_blocks[0]);     
    
    // Write self directory to disk 
    write_segment(slf_dir_data, slf_dir_inode, slf_dir_inode->file_type); 
    print_bitmap(bitmap);  
    
    // Update parent directory data block
    prt_dir_data = calloc(1, BLOCK_SIZE * BLOCKS_PER_SEGMENT); 
    read_block(superblock->disk_fd, prt_dir_data, prt_dir_inode->dir_blocks[0]); 
    i = 0; 
    if (prt_dir_inode->link_count <= N_DIRECT){
        if (!lfs_dir_entry(prt_dir_data->blocks->dir_list, path_name)){
            lfs_dir_entry(prt_dir_data->blocks + 1,  ".");        
            lfs_dir_entry(prt_dir_data->blocks + 1, "..");
            prt_dir_data->blocks->dir_list[1].inode_num = prt_dir_inode->inode_num; 
            lfs_dir_entry(prt_dir_data->blocks->dir_list, path_name);
            prt_dir_data->blocks->dir_list[2].inode_num = prt_dir_inode->inode_num + 1; 

            // Create parent directory inode block 
            prt_dir_inode = lfs_create_inode(prt_dir_inode, DATA); 
            prt_dir_inode->data_size += (uint32_t) BLOCK_SIZE + DIR_SIZE * 3; 
            prt_dir_inode->link_count += 1;
            prt_dir_inode->dir_blocks[(int) read_inode(prt_dir_inode, prt_dir_inode->link_count)]= slf_dir_inode->inode_num; 

            // Write new parent directory to disk 
            write_segment(prt_dir_data, prt_dir_inode, prt_dir_inode->file_type); 
            print_bitmap(bitmap);
            printf("superblock blocks count: %"PRIu32"\n", superblock->diskblocks_num);
        }
        else{
            // Update parent directory data block
            prt_dir_data->blocks->dir_list[(int) read_dir(prt_dir_inode, prt_dir_inode->link_count)].inode_num = slf_dir_inode->inode_num;

            // Update parent directory inode block 
            prt_dir_inode->data_size += (uint32_t) DIR_SIZE; 
            prt_dir_inode->link_count += 1;
            prt_dir_inode->dir_blocks[(int) read_inode(prt_dir_inode, prt_dir_inode->link_count)]= slf_dir_inode->inode_num;

            // Write parent directory to disk 
            write_block(superblock->disk_fd, prt_dir_data, prt_dir_inode->dir_blocks[0]);
            write_block(superblock->disk_fd, prt_dir_inode, prt_dir_inode->inode_num);
        }

    }
    
    curr = slf_dir_inode; 
    
    free(path_name);
    path_name = NULL;
    free(slf_dir_data); 
    slf_dir_data = NULL; 
    free(prt_dir_data);
    prt_dir_data = NULL;
    free(prt_dir_inode);
    prt_dir_inode = NULL;
    
    return curr; 
}

char* lfs_read_file(inode_t* curr, const char* path){
    segment_t* file_data = NULL;
    inode_t* dir_inode = NULL;
    char* buffer = NULL; 
    char* path_name = NULL; 
    int path_length = 0;
    int file_size = 0; 
    int blocks_needed = 0; 
    int inode_id = 0; 
    int i = 0;

    // Reading block (memcpy)
    dir_inode = calloc (1, BLOCK_SIZE);
    read_block(superblock->disk_fd, dir_inode, inode_id = find_dir(curr, path, curr->link_count));
    
    // Directory path reading 
    if (path == NULL){
        printf("LLFS ERROR: No path name entered\n"); 
        return NULL; 
    }
    path_length = strlen(path) + 1; // up to but not include '\0'
    path_name = (char*) calloc (1, sizeof(char) * (path_length));  
    strcpy (path_name, path); 
    
    if (path_length > MAX_FILENAME){
        printf("LLFS ERROR: %s path name exceeding 30 characters limit\n", path_name); 
        return NULL; 
    }
    for (i = 0; i < path_length; i++){
        if (!strcmp(&path_name[i], "/")){
            printf("LLFS ERROR: %s path name containing invalid '/' character\n", path_name); 
            return NULL; 
        }
    }
    // Existing directory checking 
    if (!inode_id){
        printf("LLFS file name not found\n"); 
        printf("LLFS require creating the same file before reading\n");
        return NULL; 
    }  
   
    // Read existing file 
    int fd = open(path_name, O_RDONLY, 0777); // |O_PATH
    if (fd){
        lseek(fd, 0, SEEK_SET); 
        file_size = lseek(fd, 0, SEEK_END); // 00200000 
        lseek(fd, 0, SEEK_SET); 
        // Initialize file data blocks 
        if (file_size % BLOCK_SIZE == 0) {
            blocks_needed = file_size/BLOCK_SIZE;
        }
        else{
            blocks_needed = file_size/BLOCK_SIZE + 1;
        }
        file_data = calloc(1, BLOCK_SIZE * BLOCKS_PER_SEGMENT);     

        // Read file data blocks
        read_segment (file_data, dir_inode, DATA, inode_id); 
    }
    
    free(path_name); 
    path_name = NULL;
    free(dir_inode);
    dir_inode = NULL; 

    return file_data->blocks; 
}

inode_t* lfs_write_file(inode_t* curr, char* buffer, char* path){
    segment_t* slf_file_data = NULL; 
    segment_t* prt_dir_data = NULL; 
    inode_t* slf_file_inode = NULL; 
    inode_t* prt_dir_inode = NULL;
    char* path_name = NULL; 
    int path_length = 0;
    int file_size = 0;
    int blocks_needed =0; 
    int dir_ptr = 0;
    int i = 0;

    prt_dir_inode = calloc (1, BLOCK_SIZE);
    read_block(superblock->disk_fd, prt_dir_inode, curr->inode_num);
    
    // Directory path reading 
    if (path == NULL){
        printf("LLFS ERROR: No path name entered\n"); 
    }
    path_length = strlen(path) + 1; // up to but not include '\0'
    path_name = (char*) calloc (1, sizeof(char) * (path_length));  
    strcpy (path_name, path); 
    
    if (path_length > MAX_FILENAME){
        printf("LLFS ERROR: %s path name exceeding 30 characters limit\n", path_name); 
        return NULL;
    }
    for (i = 0; i < path_length; i++){
        if (!strcmp(&path_name[i], "/")){
            printf("LLFS ERROR: %s path name containing invalid '/' character\n", path_name); 
            return NULL;    
        }
    }
    // Existing directory checking 
    if (find_dir(curr, path_name, curr->link_count)){
        printf("LLFS directory name found to exist\n"); 
        printf("LLFS require removing existing directory and associated child directories before creating the same directory\n");
        return NULL; 
    }  
        
    // Create new data file 
    int fd = open(path_name, O_CREAT | O_WRONLY, 0777); // |O_PATH
    while(buffer[i] != '\0'){ 
        i++; 
    }
    file_size = i + 1; 
    printf("file size: %d\n", file_size); 
    if(write(fd, buffer, file_size) != file_size){
        printf("LLFS ERROR: File writing error\n");
    } 
    if (file_size > MAX_FILESIZE){
        printf("LLFS error: exceeded the per data file limit > 128,000 bytes\n");
        return NULL; 
    }   
    // Initialize file data block 
    if (file_size % BLOCK_SIZE == 0) {
        blocks_needed = file_size/BLOCK_SIZE;
    }
    else{
        blocks_needed = file_size/BLOCK_SIZE + 1;
    }
    
    slf_file_data = calloc(1, BLOCK_SIZE * BLOCKS_PER_SEGMENT);
    for (i = 0; i < blocks_needed; i++){     
        memcpy(slf_file_data->blocks + i, buffer + i * BLOCK_SIZE, BLOCK_SIZE); 
    }

    // Initialize file inode block 
    slf_file_inode = lfs_create_inode(slf_file_inode, DATA); 
    slf_file_inode->data_size = file_size; 
    slf_file_inode->link_count = blocks_needed;
    slf_file_inode->dir_blocks[0] = find_firstfreeblock(bitmap); 
    printf("file_data: %"PRIu16"\n", slf_file_inode->dir_blocks[0]); 
    
    // Write data file to disk 
    write_segment(slf_file_data, slf_file_inode, slf_file_inode->file_type); 
    print_bitmap(bitmap);
    
    // Update parent directory data block
    prt_dir_data = calloc(1, BLOCK_SIZE * BLOCKS_PER_SEGMENT); 
    read_block(superblock->disk_fd, prt_dir_data, prt_dir_inode->dir_blocks[0]); 
    if (prt_dir_inode->link_count <= N_DIRECT){
        if (!lfs_dir_entry(prt_dir_data->blocks->dir_list, path_name)){
            lfs_dir_entry(prt_dir_data->blocks + 1,  ".");        
            lfs_dir_entry(prt_dir_data->blocks + 1, "..");
            prt_dir_data->blocks->dir_list[1].inode_num = prt_dir_inode->inode_num; 
            lfs_dir_entry(prt_dir_data->blocks->dir_list, path_name);
            prt_dir_data->blocks->dir_list[2].inode_num = prt_dir_inode->inode_num + 1; 
            
            // Update parent directory inode block 
            prt_dir_inode = lfs_create_inode(prt_dir_inode, DATA); 
            prt_dir_inode->data_size += (uint32_t) BLOCK_SIZE + DIR_SIZE * 3; 
            prt_dir_inode->link_count += 1;
            prt_dir_inode->dir_blocks[(int) read_inode(prt_dir_inode, prt_dir_inode->link_count)]= slf_file_inode->inode_num; 
            
            // Write new parent directory to disk 
            write_segment(prt_dir_data, prt_dir_inode, prt_dir_inode->file_type); 
            print_bitmap(bitmap);
            printf("superblock blocks count: %"PRIu32"\n", superblock->diskblocks_num);
        }
        else{
            // Update parent directory data block
            prt_dir_data->blocks->dir_list[(int) read_dir(prt_dir_inode, prt_dir_inode->link_count)].inode_num = slf_file_inode->inode_num;

            // Update parent directory inode block 
            prt_dir_inode->data_size += (uint32_t) DIR_SIZE; 
            prt_dir_inode->link_count += 1;
            prt_dir_inode->dir_blocks[(int) read_inode(prt_dir_inode, prt_dir_inode->link_count)]= slf_file_inode->inode_num;

            // Write parent directory to disk 
            write_block(superblock->disk_fd, prt_dir_data, prt_dir_inode->dir_blocks[0]);
            write_block(superblock->disk_fd, prt_dir_inode, prt_dir_inode->inode_num);
        }
    }

    curr = prt_dir_inode; 
    
    free(path_name); 
    path_name = NULL;
    free(slf_file_data);
    slf_file_data = NULL; 
    free(slf_file_inode);
    slf_file_inode = NULL; 
    free(prt_dir_data);
    prt_dir_data = NULL;
    
    return curr; 
}

void InitLLFS(){
    printf("LLFS reformating vdisk...\n");
    lfs_initialize("vdisk");
    printf("LLFS wiped vdisk clean\n");
}


void lfs_display (disk_t* vdisk){ 
    printf("block_t: %zu bytes\n", sizeof(block_t)); 
    printf("superblock_t: %zu bytes\n", sizeof(superblock_t)); 
    printf("bitmap_t: %zu bytes\n", sizeof(bitmap_t)); 
    printf("cr_metadata_t: %zu bytes\n", sizeof(cr_metadata_t));
    printf("imap_t: %zu bytes\n", sizeof(imap_t));
    printf("cr_t: %zu bytes\n", sizeof(cr_t)); 
    printf("dir_t: %zu bytes\n", sizeof(dir_t)); 
    printf("inode_t: %zu bytes\n", sizeof(inode_t)); 
    printf("seg_metadata_t: %zu bytes\n", sizeof(seg_metadata_t)); 
    printf("segment_t: %zu bytes\n", sizeof(segment_t)); 
    printf("vdisk: %zu bytes\n\n", sizeof(disk_t)); 
    
    printf("|----------------------------------------------------------------------------------------|\n"); 
    printf("|            |           |           CR            |                log                  |\n"); 
    printf("| superblock |  bitmap   |    CR meta /  CR imap   |  segment    (SS)       |     ...    |\n"); 
    printf("|  %ld bytes  |%ld bytes  | %ld bytes / %ld bytes   | %ld bytes(%ld bytes) |    ...     |\n", 
    sizeof(vdisk->superblock), sizeof(vdisk->bitmap), sizeof(vdisk->cr_meta), sizeof(vdisk->cr_imap), sizeof(segment_t), sizeof(seg_metadata_t)); 
    printf("|            |           |                         |              %ld bytes          |\n", 
    sizeof(vdisk->log)); 
    printf("|----------------------------------------------------------------------------------------|\n");
    
}

void lfs_usage(){
    fprintf(stderr,"DISPLAY <disk_name>\n");
    fprintf(stderr,"HEX <disk_name>\n");
    fprintf(stderr,"InitLLFS <disk_name>\n");
    fprintf(stderr,"EXIT\n\n");
}
