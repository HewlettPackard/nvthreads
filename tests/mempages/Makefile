CC = g++
CFLAGS = -g
PLIB = -lpthread
DLIB = ../../libnvthread.so


all:	checkpages dtests ptests

checkpages:	
	$(CC) $(CFLAGS) checkpages.c -o checkpages.o

ptests:
	$(CC) $(CFLAGS) test1.c -o ptest1.o $(PLIB)
	$(CC) $(CFLAGS) test2.c -o ptest2.o $(PLIB)

dtests:
	$(CC) $(CFLAGS) test1.c	-rdynamic $(DLIB) -o dtest1.o
	$(CC) $(CFLAGS) test2.c -rdynamic $(DLIB) -o dtest2.o

clean:
	rm *.o MemLog*
