BUILD_DIR = build

all: compileHighres compileLowres compileDateTime compileFemtoVsnprintf

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


test: all
	./$(BUILD_DIR)/testImageDrawerHighres
	./$(BUILD_DIR)/testImageDrawerLowres
	./$(BUILD_DIR)/testDateTime
	./$(BUILD_DIR)/testFemtoVsnprintf

clean:
	rm -f $(BUILD_DIR)/*
	rmdir $(BUILD_DIR)

