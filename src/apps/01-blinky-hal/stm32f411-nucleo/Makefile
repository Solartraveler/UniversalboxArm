CHIP=stm32f411xe
BOARD=nucleo

all:
	CHIP=$(CHIP) BOARD=$(BOARD) make -j -C ..

clean:
	make -C .. clean

upload:
	st-flash write ../build/$(CHIP)-$(BOARD)/*flash.bin 0x8000000
	st-flash reset
