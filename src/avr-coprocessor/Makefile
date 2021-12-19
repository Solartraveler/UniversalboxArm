all:
	$(MAKE) -C 01-minimal-reset-button
	$(MAKE) -C 01-minimal-reset-button/pc-simulator
	$(MAKE) -C 02-test-everything
	$(MAKE) -C 02-test-everything/pc-simulator

clean:
	$(MAKE) clean -C 01-minimal-reset-button
	$(MAKE) clean -C 01-minimal-reset-button/pc-simulator
	$(MAKE) clean -C 02-test-everything
	$(MAKE) clean -C 02-test-everything/pc-simulator
