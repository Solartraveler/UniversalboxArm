/*Loader for 128x128, 160x128 and 320x240 LCD*/
/* Do not edit! This file is autogenerated by MenuEdit on 2022-11-20 22:34*/

#ifndef MENU_INTERPRETER_CONFIG_H
#define MENU_INTERPRETER_CONFIG_H

#define MENU_BYTECODE_VERSION 5

#define USE16BITADDR

#define LARGESCREEN

#define MENU_DATASIZE 1840
#define MENU_OBJECTS_MAX 20
#define MENU_LIST_MAX 2
#define MENU_TEXT_MAX 13
#define MENU_CHECKBOX_MAX 2
#define MENU_RADIOBUTTON_MAX 0
#define MENU_GFX_MAX 1
#define MENU_MULTIGFX_MAX 0
#define MENU_ACTION_MAX 19

#define MENU_SCREEN_X 320
#define MENU_SCREEN_Y 240

#define MENU_SCREEN_COLORCUSTOM1

#define MENU_COLOR_CUSTOM1_RED_BITS 1
#define MENU_COLOR_CUSTOM1_GREEN_BITS 1
#define MENU_COLOR_CUSTOM1_BLUE_BITS 1
#define MENU_COLOR_CUSTOM2_RED_BITS 5
#define MENU_COLOR_CUSTOM2_GREEN_BITS 6
#define MENU_COLOR_CUSTOM2_BLUE_BITS 5
#define MENU_COLOR_CUSTOM3_RED_BITS 8
#define MENU_COLOR_CUSTOM3_GREEN_BITS 8
#define MENU_COLOR_CUSTOM3_BLUE_BITS 8

#define MENU_COLOR_BACKGROUND 0x000007
#define MENU_COLOR_BORDER 0x000000
#define MENU_COLOR_TEXT 0x000000
#define MENU_COLOR_SUBWINDOW_BACKGROUND 0x000007
#define MENU_COLOR_SUBWINDOW_BORDER 0x000000

#define MENU_ACTION_BINLOAD 1
#define MENU_ACTION_INDEXCHANGE_BINLOAD 2
#define MENU_ACTION_IMAGELEAVE 3
#define MENU_ACTION_BINEXEC 5
#define MENU_ACTION_BINWATCHDOG 7
#define MENU_ACTION_BINAUTOSTART 9
#define MENU_ACTION_BINSAVEDELETE 11
#define MENU_ACTION_BININFO 13
#define MENU_ACTION_IMAGEENTER 15
#define MENU_ACTION_INDEXCHANGE_IMAGEENTER 16
#define MENU_ACTION_STORAGEFORMAT 17

#define MENU_TEXT_BINLIST 0
#define MENU_TEXT_BINNAME 1
#define MENU_TEXT_BINVERSION 2
#define MENU_TEXT_BINAUTHOR 3
#define MENU_TEXT_BINLICENSE 4
#define MENU_TEXT_BINDATE 5
#define MENU_TEXT_SAVEDELETE 6
#define MENU_TEXT_BININFO 7
#define MENU_TEXT_LOADERVERSION 8
#define MENU_TEXT_DISKSIZE 9
#define MENU_TEXT_DISKFREE 10
#define MENU_TEXT_DISKSECTORS 11
#define MENU_TEXT_DISKCLUSTER 12

#define MENU_CHECKBOX_BINWATCHDOG 0
#define MENU_CHECKBOX_BINAUTOSTART 1


#define MENU_LISTINDEX_BININDEX 0
#define MENU_LISTINDEX_BININFO 1

#define MENU_GFX_BINDRAWING 0

#define MENU_GFX_SIZEX_BINDRAWING 320
#define MENU_GFX_SIZEY_BINDRAWING 240

#define MENU_GFX_FORMAT_BINDRAWING COLORCUSTOM1


#define MENU_USE_BOX
#define MENU_USE_LABEL
#define MENU_USE_BUTTON
#define MENU_USE_GFX
#define MENU_USE_LIST
#define MENU_USE_CHECKBOX
#define MENU_USE_SUBWINDOW
#define MENU_USE_WINDOW
#define MENU_USE_SHORTCUT

#define MENU_USE_FONT_4
#define MENU_USE_FONT_6
#define MENU_USE_FONT_MAX 6
#define MENU_USE_UTF8
#define MENU_USE_GFXFORMAT_2

#define MENU_SDATA_1_1 1649
#define MENU_SDATA_5_1 1660
#define MENU_SDATA_6_1 1660
#define MENU_SDATA_7_1 1660
#define MENU_SDATA_8_2 1676
#define MENU_SDATA_8_10 1685
#define MENU_SDATA_8_12 1693
#define MENU_SDATA_8_15 1702
#define MENU_SDATA_8_4 1709
#define MENU_SDATA_8_5 1713
#define MENU_SDATA_8_8 1722
#define MENU_SDATA_8_9 1732
#define MENU_SDATA_9_2 1676
#define MENU_SDATA_9_10 1685
#define MENU_SDATA_9_12 1693
#define MENU_SDATA_9_15 1702
#define MENU_SDATA_9_4 1709
#define MENU_SDATA_9_5 1713
#define MENU_SDATA_9_8 1722
#define MENU_SDATA_9_9 1732
#define MENU_SDATA_9_19 1737
#define MENU_SDATA_10_2 1676
#define MENU_SDATA_10_10 1685
#define MENU_SDATA_10_12 1693
#define MENU_SDATA_10_15 1702
#define MENU_SDATA_10_4 1709
#define MENU_SDATA_10_5 1713
#define MENU_SDATA_10_8 1722
#define MENU_SDATA_10_9 1732
#define MENU_SDATA_10_19 1737
#define MENU_SDATA_17_18 1676
#define MENU_SDATA_17_1 1741
#define MENU_SDATA_17_2 1747
#define MENU_SDATA_17_9 1754
#define MENU_SDATA_17_11 1763
#define MENU_SDATA_17_7 1772
#define MENU_SDATA_17_8 1777
#define MENU_SDATA_17_16 1784
#define MENU_SDATA_18_19 1676
#define MENU_SDATA_18_1 1741
#define MENU_SDATA_18_2 1747
#define MENU_SDATA_18_9 1754
#define MENU_SDATA_18_11 1763
#define MENU_SDATA_18_7 1772
#define MENU_SDATA_18_8 1777
#define MENU_SDATA_18_13 1799
#define MENU_SDATA_18_17 1784
#define MENU_SDATA_19_1 1741
#define MENU_SDATA_19_2 1747
#define MENU_SDATA_19_9 1754
#define MENU_SDATA_19_11 1763
#define MENU_SDATA_19_7 1772
#define MENU_SDATA_19_8 1777
#define MENU_SDATA_19_16 1807
#define MENU_SDATA_19_17 1800
#define MENU_SDATA_20_1 1818
#define MENU_SDATA_20_2 1833
#define MENU_SDATA_20_3 1836
#define MENU_SDATA_21_1 1818
#define MENU_SDATA_21_2 1833
#define MENU_SDATA_21_3 1836
#define MENU_SDATA_22_1 1818
#define MENU_SDATA_22_2 1833
#define MENU_SDATA_22_3 1836

#endif

