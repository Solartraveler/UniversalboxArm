COMMON=../../common
VERYCOMMON=../../../common
SIMULATOR=$(COMMON)/pc-simulator

SRC = ../main.c
SRC += $(SIMULATOR)/simulate.c

CINCS += -I..
CINCS += -I$(VERYCOMMON)
CINCS += -I$(SIMULATOR)

all:
	gcc $(SRC) -o pcSimulate -Wall -Wextra $(CINCS) -lglut -lGL -pthread -ggdb

clean:
	rm -f pcSimulate
