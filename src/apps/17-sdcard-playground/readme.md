Test characteristics of SD cards, not found in a datasheet.

SD     Controller

Vcc -> +3.3V

CS  -> PA1

DI  -> PB5

DO  -> PB4

SCK -> PB3

GND -> PA0 (via mosfet)



Example:

Read every sector until a non working one is found (speed is about 600KB/s):

command: 17 (0x11 - read single block)

argument: 0

powercycle: false

increment: 1 for SDHC/SDXC, 512 (0x200) for SD/MMC

checkread: false

stop on found: true

seek for: failure

require data: false

max transfer length: 51200

min divider: 4 (some cards might need a higher one as the transfer length can't be increased)
