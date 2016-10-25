OUT = server
CC = gcc
OPT = -lpthread -Wall

all:
	$(CC) $(OPT) ${OUT}.c -o ${OUT}
	
clean:
	rm -f ${OUT} ${OUT}.o
