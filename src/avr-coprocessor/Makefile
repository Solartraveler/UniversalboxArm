all:
	$(MAKE) -C 01-minimal-reset-button
	$(MAKE) -C 01-minimal-reset-button/pc-simulator
	$(MAKE) -C 02-test-everything
	$(MAKE) -C 02-test-everything/pc-simulator
	$(MAKE) -C 03-reset-button-with-spi
	$(MAKE) -C 03-reset-button-with-spi/pc-simulator
	$(MAKE) -C 04-charger-unittests
	$(MAKE) -C 04-charger-unittests/pc-unittest test
	$(MAKE) -C 05-spi-with-charger
	$(MAKE) -C 05-spi-with-charger/pc-simulator
	$(MAKE) -C 06-chip-bug-didr0-test

clean:
	$(MAKE) clean -C 01-minimal-reset-button
	$(MAKE) clean -C 01-minimal-reset-button/pc-simulator
	$(MAKE) clean -C 02-test-everything
	$(MAKE) clean -C 02-test-everything/pc-simulator
	$(MAKE) clean -C 03-reset-button-with-spi
	$(MAKE) clean -C 03-reset-button-with-spi/pc-simulator
	$(MAKE) clean -C 04-charger-unittests
	$(MAKE) clean -C 04-charger-unittests/pc-unittest
	$(MAKE) clean -C 05-spi-with-charger
	$(MAKE) clean -C 05-spi-with-charger/pc-simulator
	$(MAKE) clean -C 06-chip-bug-didr0-test
