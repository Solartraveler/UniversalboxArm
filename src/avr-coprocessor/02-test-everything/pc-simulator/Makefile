VERYCOMMON=../../../common
ALGORITHM=$(VERYCOMMON)/algorithm

all:
	gcc -o pcSimulate -Wall -Wextra simulate.c ../main.c $(ALGORITHM)/femtoVsnprintf.c -I. -I.. -I$(ALGORITHM) -lglut -lGL -pthread -ggdb

clean:
	rm -f pcSimulate
