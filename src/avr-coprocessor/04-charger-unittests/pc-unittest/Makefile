COMMON=../../common
VERYCOMMON=../../../common
ALGORITHM=$(VERYCOMMON)/algorithm

SRC = tests.c
SRC += ../main.c
SRC += $(ALGORITHM)/femtoVsnprintf.c
SRC += $(COMMON)/chargerStatemachine.c

CINCS = -I.
CINCS += -I..
CINCS += -I$(ALGORITHM)
CINCS += -I$(COMMON)

all:
	gcc $(SRC) -o pcTests -Wall -Wextra $(CINCS) -ggdb

test: all
	./pcTests

clean:
	rm -f pcTests
