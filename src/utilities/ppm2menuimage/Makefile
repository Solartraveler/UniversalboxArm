all:
	mkdir -p build
	gcc ppm2menuimage.c -o build/ppm2menuimage -Wall -Wextra -ggdb

test: all
	./build/ppm2menuimage            --input ../../stm32l452/01-blinky-hal/image.ppm --output ./build/testImageUncomressed.bin
	./build/ppm2menuimage --compress --input ../../stm32l452/01-blinky-hal/image.ppm --output ./build/testImageComressed.bin

clean:
	rm -r build
