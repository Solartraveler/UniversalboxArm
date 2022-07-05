COMMON=../../common
VERYCOMMON=../../../common
ALGORITHM=$(VERYCOMMON)/algorithm
SIMULATOR=$(COMMON)/pc-simulator

SRC = tests.c
SRC += ../main.c
SRC += $(ALGORITHM)/femtoVsnprintf.c
SRC += $(COMMON)/chargerStatemachine.c
SRC += $(SIMULATOR)/counter.c

CINCS = -I.
CINCS += -I..
CINCS += -I$(ALGORITHM)
CINCS += -I$(COMMON)
CINCS += -I$(SIMULATOR)

all:
	gcc $(SRC) -o pcTests -Wall -Wextra $(CINCS) -ggdb

test: all
	./pcTests

clean:
	rm -f pcTests
