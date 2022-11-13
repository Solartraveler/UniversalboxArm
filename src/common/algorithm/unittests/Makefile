all: compileHighres compileLowres compileDateTime compileFemtoVsnprintf

compileHighres:
	mkdir -p bin
	gcc -DTESTHIGHRES testImageDrawer.c ../imageDrawer.c ../imageDrawerHighres.c ../utility.c -o bin/testImageDrawerHighres

compileLowres:
	mkdir -p bin
	gcc -DTESTLOWRES testImageDrawer.c ../imageDrawer.c ../imageDrawerLowres.c ../utility.c -o bin/testImageDrawerLowres

compileDateTime:
	mkdir -p bin
	gcc testDateTime.c ../dateTime.c -o bin/testDateTime

compileFemtoVsnprintf:
	mkdir -p bin
	gcc -DTEST_PC -I. ../femtoVsnprintf.c -o bin/testFemtoVsnprintf


test: all
	./bin/testImageDrawerHighres
	./bin/testImageDrawerLowres
	./bin/testDateTime
	./bin/testFemtoVsnprintf

clean:
	rm -f bin/testImageDrawerHighres
	rm -f bin/testImageDrawerLowres
	rm -f bin/testDateTime
	rm -f bin/testFemtoVsnprintf
	rmdir bin

