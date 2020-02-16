Welcome to LLFS!

Introduction 
- LFFS is a custom virtual log structured file system for Linux. 

Disk Structure
- Virtual file systems run on top of an already existing ~/vdisk file.
- The virutal disk size is about 2,097,152 bytes (~2MB)
- The segment limit is less or equal to 128,000 bytes
- The segment limit applies to both data files and directory files 

Compliation 
- Run "make" (Note there will be warnings but it is due to the union block struct 
  with pointer casting enabled by default)
- Once complied, run "./test"

LLFS Usage
- Once the disk is mounted, LLFS is able to perform normal file system tasks
  such as create, open, read, write, and so on. 
- LLFS will be able to write files in root directory (less or equal to 128,000 bytes)
- LLFS will be able to read files in root directory 
- LLFS will be able to create subdirectories in root directory 
- LLFS will be able to create subdirectories in any directory 
- LLFS will is not able to remove files/directories but will be able to clear the disk (InitLLFS)
- LLFS contain an interative command console: 
	- [D] disk strucuture display
	- [H] disk data hex dump
	- [I] disk reformat (InitLLFS)
	- [E] LLFS exit 


LLFS Testing 
- test.c file will tests for both LLFS functionality & disk mount including:
- test for creating root directory
- test for writing files in root directory 
- test for reading files in root directory 
- test for creating disk sub-directory 
- stress test for writing files in disk sub-directory 
- stress test for reading files in disk sub-directory 
- test for disk mount 
- test for disk reformat 
- continue with LLFS interative command console

LLFS Note
- LLFS will take up to 5 seconds to initialize the vdisk


