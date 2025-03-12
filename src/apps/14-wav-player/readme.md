Plays a .wav file

Select a wave audio file. Then it can be played with some extra external components.

PWM generation is limited to 8bits, so do not expect high quality sound.

However all uncompressed 8Bit and 16Bit mono and stereo .wav files should work.

Convert files to 8 bit single channel and 8kHz samplerate with:

ffmpeg -i input.wav -ac 1 -acodec pcm_u8 -ar 8000 output8k.wav

Schematic for playback: https://www.mikrocontroller.net/articles/Klangerzeugung#Lautsprecher