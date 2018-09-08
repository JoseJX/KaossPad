all: testP3A

CC=gcc
CFLAGS=-Wall -ggdb -g3
LDFLAGS=

%.o:%.c
	${CC} ${CFLAGS} -o $@ -c $<

testP3A: P3A.o testP3A.o P3A.h
	${CC} ${CFLAGS} ${LDFLAGS} -o $@ $^	

clean:
	rm -f parseP3A *.o
