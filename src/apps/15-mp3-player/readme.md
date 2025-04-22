Plays a .mp3 file

Select a mp3 audio file. Then it can be played with some extra external components.

PWM generation is limited to 8bits, so do not expect high quality sound.

Tested for mp3 files with up to 128kBit/s datarate.

sox -n -r 24000  "$(BUILD_DIR)/sine-600Hz-1ch-24k.mp3" synth 5 sine 600

Schematic for playback: https://www.mikrocontroller.net/articles/Klangerzeugung#Lautsprecher

The LED 1 lights red if the output FIFO underruns.

Performance data are printed to the serial port at the end of a file, not if manually stopped.