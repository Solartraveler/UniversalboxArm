.PHONY: all \
	01-blinky-hal \
	01-blinky-hal-stm32f411-nucleo \
	02-test-everything-hal \
	02-test-everything-hal-stm32f411-nucleo \
	03-loader \
	03-loader-pc-simulator \
	04-coprocessor-uart-forward \
	04-coprocessor-uart-forward-pc-simulator \
	05-coprocessor-control \
	05-coprocessor-control-pc-simulator \
	06-usb-mass-storage \
	07-ntp-clock \
	07-ntp-clock-pc-simulator \
	08-freertos-helloworld \
	08-freertos-helloworld-pc-simulator \
	09-gamebox-pc-simulator \
	09-gamebox-avr \
	09-gamebox-arm \
	10-infrared-decoder \
	11-adc-scope \
	11-adc-scope-pc-simulator \
	11-adc-scope-stm32f411-nucleo \
	12-usb-cdc-loop \
	13-battery-capacity-tester \
	13-battery-capacity-tester-pc-simulator \
	14-wav-player \
	14-wav-player-pc-simulator \
	15-mp3-player \
	15-mp3-player-pc-simulator \
	16-led-matrix-tester \
	17-sdcard-playground \
	18-dac-tester

all: \
	01-blinky-hal \
	01-blinky-hal-stm32f411-nucleo \
	02-test-everything-hal \
	02-test-everything-hal-stm32f411-nucleo \
	03-loader \
	03-loader-pc-simulator \
	04-coprocessor-uart-forward \
	04-coprocessor-uart-forward-pc-simulator \
	05-coprocessor-control \
	05-coprocessor-control-pc-simulator \
	06-usb-mass-storage \
	07-ntp-clock \
	07-ntp-clock-pc-simulator \
	08-freertos-helloworld \
	08-freertos-helloworld-pc-simulator \
	09-gamebox-pc-simulator \
	09-gamebox-avr \
	09-gamebox-arm \
	10-infrared-decoder \
	11-adc-scope \
	11-adc-scope-pc-simulator \
	11-adc-scope-stm32f411-nucleo \
	12-usb-cdc-loop \
	13-battery-capacity-tester \
	13-battery-capacity-tester-pc-simulator \
	14-wav-player \
	14-wav-player-pc-simulator \
	15-mp3-player \
	15-mp3-player-pc-simulator \
	16-led-matrix-tester \
	17-sdcard-playground \
	18-dac-tester


01-blinky-hal:
	$(MAKE) -C 01-blinky-hal

01-blinky-hal-stm32f411-nucleo:
	$(MAKE) -C 01-blinky-hal/stm32f411-nucleo

02-test-everything-hal:
	$(MAKE) -C 02-test-everything-hal

02-test-everything-hal-stm32f411-nucleo:
	$(MAKE) -C 02-test-everything-hal/stm32f411-nucleo

03-loader:
	$(MAKE) -C 03-loader

03-loader-pc-simulator:
	$(MAKE) -C 03-loader/pc-simulator

04-coprocessor-uart-forward:
	$(MAKE) -C 04-coprocessor-uart-forward

04-coprocessor-uart-forward-pc-simulator:
	$(MAKE) -C 04-coprocessor-uart-forward/pc-simulator

05-coprocessor-control:
	$(MAKE) -C 05-coprocessor-control

05-coprocessor-control-pc-simulator:
	$(MAKE) -C 05-coprocessor-control/pc-simulator

06-usb-mass-storage:
	$(MAKE) -C 06-usb-mass-storage

07-ntp-clock:
	$(MAKE) -C 07-ntp-clock

07-ntp-clock-pc-simulator:
	$(MAKE) -C 07-ntp-clock/pc-simulator

08-freertos-helloworld:
	$(MAKE) -C 08-freertos-helloworld

08-freertos-helloworld-pc-simulator:
	$(MAKE) -C 08-freertos-helloworld/pc-simulator

09-gamebox-pc-simulator:
	$(MAKE) -C 09-gamebox/pc-simulator

09-gamebox-avr:
	$(MAKE) -C 09-gamebox/avr

09-gamebox-arm:
	$(MAKE) -C 09-gamebox/arm

10-infrared-decoder:
	$(MAKE) -C 10-infrared-decoder

11-adc-scope:
	$(MAKE) -C 11-adc-scope

11-adc-scope-pc-simulator:
	$(MAKE) -C 11-adc-scope/pc-simulator

11-adc-scope-stm32f411-nucleo:
	$(MAKE) -C 11-adc-scope/stm32f411-nucleo

12-usb-cdc-loop:
	$(MAKE) -C 12-usb-cdc-loop

13-battery-capacity-tester:
	$(MAKE) -C 13-battery-capacity-tester

13-battery-capacity-tester-pc-simulator:
	$(MAKE) -C 13-battery-capacity-tester/pc-simulator

14-wav-player:
	$(MAKE) -C 14-wav-player

14-wav-player-pc-simulator:
	$(MAKE) -C 14-wav-player/pc-simulator

15-mp3-player:
	$(MAKE) -C 15-mp3-player

15-mp3-player-pc-simulator:
	$(MAKE) -C 15-mp3-player/pc-simulator

16-led-matrix-tester:
	$(MAKE) -C 16-led-matrix-tester

17-sdcard-playground:
	$(MAKE) -C 17-sdcard-playground

18-dac-tester:
	$(MAKE) -C 18-dac-tester

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
	$(MAKE) clean -C 11-adc-scope/pc-simulator
	$(MAKE) clean -C 12-usb-cdc-loop
	$(MAKE) clean -C 13-battery-capacity-tester
	$(MAKE) clean -C 13-battery-capacity-tester/pc-simulator
	$(MAKE) clean -C 14-wav-player
	$(MAKE) clean -C 14-wav-player/pc-simulator
	$(MAKE) clean -C 15-mp3-player
	$(MAKE) clean -C 15-mp3-player/pc-simulator
	$(MAKE) clean -C 16-led-matrix-tester
	$(MAKE) clean -C 17-sdcard-playground
	$(MAKE) clean -C 18-dac-tester
