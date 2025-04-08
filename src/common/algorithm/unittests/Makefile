BUILD_DIR = build

CFLAGS += -fsanitize=address -Wall
LDFLAGS += -fsanitize=address

all: compileHighres compileLowres compileDateTime compileFemtoVsnprintf compileTgaWrite compileLocklessfifo

buildDir:
	mkdir -p $(BUILD_DIR)

compileHighres: buildDir
	gcc -DTESTHIGHRES testImageDrawer.c ../imageDrawer.c ../imageDrawerHighres.c ../utility.c -o $(BUILD_DIR)/testImageDrawerHighres

compileLowres: buildDir
	gcc -DTESTLOWRES testImageDrawer.c ../imageDrawer.c ../imageDrawerLowres.c ../utility.c -o $(BUILD_DIR)/testImageDrawerLowres

compileDateTime: buildDir
	gcc testDateTime.c ../dateTime.c -o $(BUILD_DIR)/testDateTime

compileFemtoVsnprintf: buildDir
	gcc -DTEST_PC -I. ../femtoVsnprintf.c -o $(BUILD_DIR)/testFemtoVsnprintf

compileTgaWrite: buildDir
	gcc $(CFLAGS) -c testImageTgaWrite.c -o $(BUILD_DIR)/testImageTgaWrite.o
	gcc $(CFLAGS) -c ../imageTgaWrite.c -o $(BUILD_DIR)/imageTgaWrite.o
	gcc $(LDFLAGS) $(BUILD_DIR)/imageTgaWrite.o $(BUILD_DIR)/testImageTgaWrite.o -o $(BUILD_DIR)/testTgaWrite

compileLocklessfifo: buildDir
	gcc $(CFLAGS) testLocklessfifo.c -o $(BUILD_DIR)/testLocklessfifo

test: all
	./$(BUILD_DIR)/testImageDrawerHighres
	./$(BUILD_DIR)/testImageDrawerLowres
	./$(BUILD_DIR)/testDateTime
	./$(BUILD_DIR)/testFemtoVsnprintf
	./$(BUILD_DIR)/testTgaWrite $(BUILD_DIR)
	identify $(BUILD_DIR)/*.tga
	./$(BUILD_DIR)/testLocklessfifo

clean:
	rm -f $(BUILD_DIR)/*
	rmdir $(BUILD_DIR)

