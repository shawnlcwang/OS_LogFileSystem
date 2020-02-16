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

// Testing LLFS functionality 
int main(int argc, char** argv) {
    block_t* dir = NULL;
    inode_t* curr = NULL; 
    segment_t* segment_buf = NULL;
    char command[MAX_LINESIZE];
    char* buffer; 
    int i = 0, j = 0; 
    
    // LLFS vidsk initialization 
    printf("LLFS initialization...\n");
    lfs_initialize("vdisk");
    printf("LLFS initialized vdisk\n");
    
    // LLFS test inode initialization 
    curr = calloc (1, BLOCK_SIZE);
    
    // LLFS root directory setup 
    curr = lfs_mkroot();
    printf("LLFS setup root ./ directory\n");

    // LLFS root directory writing files
    buffer = calloc (1, 1024); 
    for (i = 0; i < 1023; i++){
        buffer[i]= 'h'; 
        //printf("%d: %c\n", i, buffer[i]); 
    } 
    buffer[i] = '\0'; 
    //printf("%d: %c EOF\n", i, buffer[i]);
    curr = lfs_write_file(curr, buffer, "root_test"); 
    printf("LLFS wrote root_test file in root directory to vdisk\n");
    //memcpy(curr, curr, BLOCK_SIZE);
    
    // LLFS root directory reading files
    segment_buf = (segment_t*) lfs_read_file(curr, "root_test"); 
    printf("LLFS read root_test file in root directory from vdisk\n");
    buffer = NULL; 
    buffer = (char*) segment_buf->blocks; 
    printf("%s\n", buffer);
    
    //LLFS root directory creating sub-directory
    curr = lfs_mkdir(curr, "disk"); 
    printf("LLFS created disk sub-directory in root directory\n");
    
    // LLFS disk directory writing files
    buffer = calloc (1, 127500); 
    for (i = 0; i < 127499; i++){
        buffer[i]= 'i'; 
    } 
    buffer[i] = '\0'; 
    curr = lfs_write_file(curr, buffer, "disk_test"); 
    printf("LLFS wrote disk_test file in disk sub-directory to vdisk\n");
    
    // LLFS disk directory reading files
    segment_buf = (segment_t*) lfs_read_file(curr, "disk_test"); 
    printf("LLFS read root_test file in disk sub-directory from vdisk\n");
    buffer = NULL; 
    buffer = (char*) segment_buf->blocks; 
    printf("%s\n", buffer);
          
    //lfs_display(&vdisk);   
    lfs_usage();
    
    printf("ENTER COMMAND: \n");
    while (fgets(command, MAX_LINESIZE, stdin)){ //default internal terminal -> changed to stdout
        switch (command[0]){
            case 'H': 
                system("hexdump -C vdisk");  // INT32 - Little Endian DCBA use Run NOT debugger!!!!
				printf("ENTER COMMAND: \n");
                break; 
            case 'D': 
                lfs_display(&vdisk); 
				printf("ENTER COMMAND: \n");
                break; 
            case 'I':
                InitLLFS();
				printf("ENTER COMMAND: \n");
                break;                 
            case 'E':
                return (EXIT_SUCCESS);
                break;              
            default: 
                lfs_usage(); 
				printf("ENTER COMMAND: \n");
                continue; 
        }
    }
    close(superblock->disk_fd); 
    free(superblock);
    free(bitmap);
    free(cr_meta); 
    free(cr_imap);
    free(buffer);
    free(segment_buf);
    free(curr);
    return (EXIT_SUCCESS);
}