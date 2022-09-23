all: compileHighres compileLowres

compileHighres:
	mkdir -p bin
	gcc -DTESTHIGHRES testImageDrawer.c ../imageDrawer.c ../imageDrawerHighres.c ../utility.c -o bin/testImageDrawerHighres

compileLowres:
	mkdir -p bin
	gcc -DTESTLOWRES testImageDrawer.c ../imageDrawer.c ../imageDrawerLowres.c ../utility.c -o bin/testImageDrawerLowres


test: compileHighres compileLowres
	./bin/testImageDrawerHighres
	./bin/testImageDrawerLowres

clean:
	rm -f bin/testImageDrawerHighres
	rm -f bin/testImageDrawerLowres
	rmdir bin

