COMMON=../../common
VERYCOMMON=../../../common
ALGORITHM=$(VERYCOMMON)/algorithm
SIMULATOR=$(COMMON)/pc-simulator

SRC = ../main.c
SRC += $(SIMULATOR)/simulate.c
SRC += $(SIMULATOR)/counter.c
SRC += $(ALGORITHM)/femtoVsnprintf.c
SRC += $(COMMON)/chargerStatemachine.c

CINCS = -I.
CINCS += -I..
CINCS += -I$(ALGORITHM)
CINCS += -I$(COMMON)
CINCS += -I$(VERYCOMMON)
CINCS += -I$(SIMULATOR)

all:
	gcc $(SRC) -o pcSimulate -Wall -Wextra $(CINCS) -lglut -lGL -pthread -ggdb

clean:
	rm -f pcSimulate
