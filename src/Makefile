# botstasiu - a simple chess engine written in c.
# See LICENSE file for copyright and license details.
.POSIX:

CC = cc
CFLAGS = -std=c99 -Wall -Wextra -pedantic -Wno-deprecated-declarations -Wno-implicit-fallthrough -Ofast

REQ = bitboards evaluate misc movegen moveorder position search tt uci

all: main

main.o: main.c ${REQ:=.h}

.c.o:
	${CC} -o $@ -c ${CFLAGS} $<

main: main.o ${REQ:=.o} chesslib.h
	${CC} -o $@ ${REQ:=.o} main.o

clean:
	rm -f main main.o ${REQ:=.o}
