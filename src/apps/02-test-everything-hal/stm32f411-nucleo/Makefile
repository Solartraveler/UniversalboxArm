CHIP=stm32f411xe
BOARD=nucleo

#The app is too large for the RAM, so it can only run from the flash
all:
	CHIP=$(CHIP) BOARD=$(BOARD) make -C .. build-flash

clean:
	make -C .. clean

upload:
	st-flash write ../build/$(CHIP)-$(BOARD)/*flash.bin 0x8000000
	st-flash reset
