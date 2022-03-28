======= MenuDesigner ==========
MenuEdit Version 2.0
MenuInterpreter Version 2.0
Release Date: Svn version, not released

(c) 2009-2010, 2012, 2014 - 2016, 2019, 2020 by Malte Marwedel
http://www.marwedels.de/malte

Terms of use:
MenuEditor and samples: GPL version 2.0 or later
MenuInterpreter: Mozilla Public License version 2.0.

So it is possible to use the interpreter in non open source applications as long as changes to the interpreter are made public under the same terms.

Some parts of the testing application uses code which is public domain.
The stm-demo uses code under the terms of the BSD 3-Clause license and Apache-2.0 license.

======== Fast Howto ==========
Generate a menu with MenuEdit. Save the file as .xml. Export the menu to the directory
menuInterpreter/pc-demo. Existing files will be overwritten.
Change into the menuInterpreter/pc-demo directory and call make.
run ./simple-demo
Hint: In order to send a char to the program use Control+D instead of the return key.
There is a more detailed Howto in the file fast-start-hello-world.pdf

======= Features ============
Up to 16MB menu size in 24/32 Bit addressing mode
Up to 64KB in 16 bit addressing mode
Store the menu on any random access readable memory
Recompilation not needed for small changes like typo fixes or x,y position changes
Objects with focus
Optimised for black/white LCDs up to a size of 4096x4096 pixel
Up to 255 different inputs 'key presses' per screen
Up to 2^16-1 different output 'actions' per screen
Low permanent RAM usage: 7 bytes + 4*Addresssize + Addresssize*(max objects on a window/subwindow) + (5 bytes if UTF-8 is enabled)
No increase in CPU consumption with more screens (expect perhaps for the access time for the storage memory)
Data for objects can either be stored statically in the menu or for dynamic data in the RAM
12 fonts included (5x7, 5x7 shrinked, 5x7 underlined, 5x7 shrinked and underlined, 5x5 in various shrinked forms, 8x15 in four variants as 5x7)
Code can run on AVR, PC and any other platform supported by a C compiler supporting WEAK keyword
Amount of objects per screen is only limited by memory and computing time
Partially mouse/touchscreen support (missing for the List)
Multi-language support
UTF-8 support
Generateing window transition overview with graphviz
Support for 7 segment displays
Support for 4Bit greyscale displays
Support for color displays with up to 8bits per R, G, B channel

Objects:
Window: Can contain objects
SubWindow: Good for pop-up messages, has all abilities of a normal window
Box: Draws a box either white or black
Label: Draws text on the screen
Button: A button for actions or text with a border
Graphic: Black and white pixel images, supports basic compression
List: Up to 30000 elements per list, up to 256 different lists, page up, page down key support
Checkbox: Up to 255 different checkboxes
Radiobutton: Up to 16 different button groups with up to 16 different buttons per group
Shortcut: Invisible, Supports direct actions and window changes
Global shortcut: Like a shortcut, but once defined, its valid within all windows

======= MenuEdit ============
Compiled and tested with Lazarus version 2.0.0+dfsg-2 under Debian Linux, 64bit
Compiled and tested with Lazarus version 2.0.6 under Windows 10, 64bit

Older versions:
Compiled and tested with Lazarus version 1.2.4 under Debian Linux, 64bit
Compiled and tested with Lazarus version 1.6 under Windows Vista, 32bit
Compiled and tested with Lazarus version 1.6 under Windows 7, 64bit

======= How to use ==========
Normally you have to implemented the following functions for yourself:

The access to the binary menu structure. Like RAM, Flash or EEPROM read.
Its best to check if 'addr' is below the MENU_DATASIZE definition before accessing.
extern uint8_t menu_byte_get(MENUADDR addr);

This function gets called while the screen is redrawn. Please note that x and y can be outside
of the defined screen size. You must catch this in order to prevent memory corruption.
The screen size can be compared with the defines MENU_SCREEN_X and MENU_SCREEN_Y.
SCREENPOS is an uint8_t or uint16_t, depending on the selected screen size.
Depending on the selected output color format, SCREENCOLOR is a uint8_t, uint16_t or uint32_t.
If an output with color is selected, the lower bits represent blue, the higher bits red.
extern void menu_screen_set(SCREENPOS x, SCREENPOS y, SCREENCOLOR color);

This function gets called once, when redrawing has completed. Use this to draw the values set by the
function above to the screen.
extern void menu_screen_flush(void);

Gets called on every screen switch. Simply provided, because there are often faster methods for clearing
a screen than drawing every single pixel.
extern void menu_screen_clear(void);

This function gets called so you can react on the user inputs of the screen, return 1 if you want a redraw,
0 if not. If a screen change occurs, a redraw is made anyway. Compare action with the MENU_ACTION_* defines to
determine the selected user input.
MENUACTION is an uint8_t if 16bit addressing is used or an uint16_t if 24bit addressing is used. In the unlikely
case of a small <64KiB menu having more than 255 actions, the menueditor switches to 24bit addressing.
extern uint8_t menu_action(MENUACTION action);

You should call the void menu_keypress(uint8_t key) function in order to initiate screen changes
or let the menu react on other key inputs.

If you have dynamic data (text, graphics) on the screen you should call void menu_redraw(void) in order
to init a redraw.

For devices with mouse or touch support, a menu_keypress can be started on a visible element with the following function:
void menu_mouse(SCREENPOS x, SCREENPOS y, uint8_t key);

With void menu_language_set(uint8_t id) more than one language can be used. Call menu_redraw() after setting the language.

The MenuEdit generates four files:
menu-interpreter-config.h: This file contains a lot of autogenerated #defines. Whenever this file changes, a recompilation
is required.
menudata.bin: The bytecodes which must be provided by the menu_byte_get() function.
menudata.c: Exactly the same content as menudata.bin, but as confortable C array.
menudata-progmem.c: Same as menudata.c, but with a const PROGMEM attribute. Useful for AVRs and other devices with separate RAM/ROM.

The pc-demo provides a way for a minimal implementation of each of the functions above.
The avr-demo provides a advanced example of all features and how to read out things like checkbox states and so on.

Depending of the size of the menudata MENUADDR is either defined as 16 or 32 bit datatype.

Call menu_language_set() with the number of the language to use. You need to call redraw() in order for the
changes to become active.

For generating graphviz dependencies, you should "apt-get install graphviz" or install graphviz-2.38.msi to the default path.
For directly displaying the generated files, firefox should be installed to the default path.

======= Hints ===============
Better not edit the .xml file for the MenuEdit by hand, because the program was written in a quick
and dirty way. Missing attributes for objects will crash the program, attributes not useful for the
object will result in an invalid output.

Use 'RET' as window switch in order to return from a subwindow to the normal window and restore the
previous focus. Leave the window switch field blank.

The compression for graphics is very weak. In the best case it saves space by a factor of eight. In the
worst case it increases the space by a factor of eight compared to the uncompressed version.

MenuEdit can open xml files given as parameter too.

====== Known bugs and useful improvements ========

Automatically add the .xml extention on saving if the user forgets it.

See menuEditor/bugs.txt for known problems.

If mouse support is used and non-focusable objects are lying above focusable ones, the focusable ones can
  still be be selected.

==== END OF FILE ====