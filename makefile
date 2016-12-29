OUT = server
CC = gcc
OPT = -pthread -Wall

all:
	$(CC) $(OPT) ${OUT}.c -o ${OUT}
	
clean:
	rm -f ${OUT} ${OUT}.o
