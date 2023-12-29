# UDEV rule

To use the box without root access under Linux, copy 55-universalboxarm.rules to /etc/udev/rules.d/

Then run

udevadm control --reload-rules

and if the box is already connected, next run

udevadm trigger --subsystem-match=usb

You might want to add a similar rule to the DFU bootloader used by ST. But probably you have already done so.
Or you can disable the rule for the bootloader for ST, to prevent the loader firmware in the flash from accidentially be overwritten.

Happy using.
