all:
	$(MAKE) -j -C 01-blinky-hal
	$(MAKE) -C 01-blinky-hal/stm32f411-nucleo
	$(MAKE) -j -C 02-test-everything-hal
	$(MAKE) -j -C 03-loader
	$(MAKE) -j -C 03-loader/pc-simulator
	$(MAKE) -j -C 04-coprocessor-uart-forward
	$(MAKE) -j -C 04-coprocessor-uart-forward/pc-simulator
	$(MAKE) -j -C 05-coprocessor-control
	$(MAKE) -j -C 05-coprocessor-control/pc-simulator
	$(MAKE) -j -C 06-usb-mass-storage
	$(MAKE) -j -C 07-ntp-clock
	$(MAKE) -j -C 07-ntp-clock/pc-simulator
	$(MAKE) -j -C 08-freertos-helloworld
	$(MAKE) -j -C 08-freertos-helloworld/pc-simulator
	$(MAKE) -j -C 09-gamebox/pc-simulator
	$(MAKE) -j -C 09-gamebox/avr
	$(MAKE) -j -C 09-gamebox/arm
	$(MAKE) -j -C 10-infrared-decoder
	$(MAKE) -j -C 11-adc-scope

clean:
	$(MAKE) clean -C 01-blinky-hal
	$(MAKE) clean -C 02-test-everything-hal
	$(MAKE) clean -C 03-loader
	$(MAKE) clean -C 03-loader/pc-simulator
	$(MAKE) clean -C 04-coprocessor-uart-forward
	$(MAKE) clean -C 04-coprocessor-uart-forward/pc-simulator
	$(MAKE) clean -C 05-coprocessor-control
	$(MAKE) clean -C 05-coprocessor-control/pc-simulator
	$(MAKE) clean -C 06-usb-mass-storage
	$(MAKE) clean -C 07-ntp-clock
	$(MAKE) clean -C 07-ntp-clock/pc-simulator
	$(MAKE) clean -C 08-freertos-helloworld
	$(MAKE) clean -C 08-freertos-helloworld/pc-simulator
	$(MAKE) clean -C 09-gamebox/pc-simulator
	$(MAKE) clean -C 09-gamebox/avr
	$(MAKE) clean -C 09-gamebox/arm
	$(MAKE) clean -C 10-infrared-decoder
	$(MAKE) clean -C 11-adc-scope