CC = gcc
JUNKF = $(OBJS) *~
JUNKD = *.dSYM
CFLAGS = -g   

default: test

test: test.o File.o vdisk.o 
	$(CC) $(CFLAGS) test.o File.o vdisk.o -o test -lm
	
File.o: File.c vdisk.h 
	$(CC) $(CFLAGS) -c File.c -lm

vdisk.o: vdisk.c File.h 
	$(CC) $(CFLAGS) -c vdisk.c -lm

clean:
	rm -rf test01 *.o *~; rm -f $(JUNKF) test; rm -rf $(JUNKD)

