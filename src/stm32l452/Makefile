all:
	$(MAKE) -j -C 01-blinky-hal
	$(MAKE) -j -C 02-test-everything-hal
	$(MAKE) -j -C 03-loader
	$(MAKE) -j -C 03-loader/pc-simulator


clean:
	$(MAKE) clean -C 01-blinky-hal
	$(MAKE) clean -C 02-test-everything-hal
	$(MAKE) clean -C 03-loader
	$(MAKE) clean -C 03-loader/pc-simulator
