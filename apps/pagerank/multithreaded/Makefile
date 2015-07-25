OBJS = main.o \
       vector.o \
       matrix.o \
       crsmatrix.o \
       crsmatrixop.o \
       algorithm.o \
       utils.o \
       logger.o

CC = gcc -ggdb -std=gnu99 -O3 -pthread -lrt
EXEC = prr
W_FLAGS = -Wall -Wextra -pedantic

FLAGS = -I . $(W_FLAGS)

%.o : %.c
	$(CC) $(FLAGS) -c $<

main: $(OBJS)
	@echo "Building $(EXEC)"
	$(CC) $(OBJS) $(FLAGS) -o $(EXEC)

tags:
	ctags -R .
		
clean:
	@echo "Cleaning all!"
	\rm -rf tags *.o *.out *.txt *.~ $(EXEC) test

test: test.c
	gcc -Wall -Wextra -std=c99 test.c -o test
