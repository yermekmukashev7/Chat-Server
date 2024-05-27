CC = gcc

CFLAGS = -Wall -Iutil -Wextra

all: ./server

./server : ./server.c
	${CC} ${CFLAGS} ./server.c ./server.h -o ./server

clean:
	rm -f ./server