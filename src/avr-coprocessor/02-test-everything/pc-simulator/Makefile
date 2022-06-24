COMMON=../../common
VERYCOMMON=../../../common
ALGORITHM=$(VERYCOMMON)/algorithm

SRC = simulate.c
SRC += counter.c
SRC += ../main.c
SRC += $(ALGORITHM)/femtoVsnprintf.c
SRC += $(COMMON)/chargerStatemachine.c

CINCS = -I.
CINCS += -I..
CINCS += -I$(ALGORITHM)
CINCS += -I$(COMMON)

all:
	gcc $(SRC) -o pcSimulate -Wall -Wextra $(CINCS) -lglut -lGL -pthread -ggdb

clean:
	rm -f pcSimulate
