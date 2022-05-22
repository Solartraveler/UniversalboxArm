all:
	$(MAKE) -j -C 01-blinky-hal
	$(MAKE) -j -C 02-test-everything-hal
	$(MAKE) -j -C 03-loader
	$(MAKE) -j -C 03-loader/pc-simulator
	$(MAKE) -j -C 04-coprocessor-uart-forward
	$(MAKE) -j -C 04-coprocessor-uart-forward/pc-simulator

clean:
	$(MAKE) clean -C 01-blinky-hal
	$(MAKE) clean -C 02-test-everything-hal
	$(MAKE) clean -C 03-loader
	$(MAKE) clean -C 03-loader/pc-simulator
	$(MAKE) clean -C 04-coprocessor-uart-forward
	$(MAKE) clean -C 04-coprocessor-uart-forward/pc-simulator
