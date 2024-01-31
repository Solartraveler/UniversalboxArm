/*ADC scope*/
/* Do not edit! This file is autogenerated by MenuEdit on 2024-01-26 22:43*/

#ifndef MENU_INTERPRETER_CONFIG_H
#define MENU_INTERPRETER_CONFIG_H

#define MENU_BYTECODE_VERSION 5

#define USE16BITADDR

#define LARGESCREEN

#define MENU_DATASIZE 1393
#define MENU_OBJECTS_MAX 23
#define MENU_LIST_MAX 3
#define MENU_TEXT_MAX 5
#define MENU_CHECKBOX_MAX 9
#define MENU_RADIOBUTTON_MAX 3
#define MENU_GFX_MAX 1
#define MENU_MULTIGFX_MAX 0
#define MENU_ACTION_MAX 33

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

#define MENU_COLOR_BACKGROUND 0x000000
#define MENU_COLOR_BORDER 0x000007
#define MENU_COLOR_TEXT 0x000007
#define MENU_COLOR_SUBWINDOW_BACKGROUND 0x000000
#define MENU_COLOR_SUBWINDOW_BORDER 0x000007

#define MENU_ACTION_XINC 1
#define MENU_ACTION_XDEC 3
#define MENU_ACTION_YINC 5
#define MENU_ACTION_YDEC 7
#define MENU_ACTION_YOFFINC 9
#define MENU_ACTION_YOFFDEC 11
#define MENU_ACTION_TRIGGERINC50 13
#define MENU_ACTION_TRIGGERDEC50 15
#define MENU_ACTION_TRIGGERSTART 17
#define MENU_ACTION_SCREENSHOT 19
#define MENU_ACTION_UPDATEINPUT 21
#define MENU_ACTION_INDEXCHANGE_UPDATEINPUT 22
#define MENU_ACTION_TRIGGERINC500 23
#define MENU_ACTION_TRIGGERDEC500 25
#define MENU_ACTION_TRIGGERUPDATE 27
#define MENU_ACTION_CONFIGSAVE 29
#define MENU_ACTION_CONFIGDEFAULT 31

#define MENU_TEXT_XAXIS 0
#define MENU_TEXT_YAXIS 1
#define MENU_TEXT_YOFF 2
#define MENU_TEXT_TRIGGER 3
#define MENU_TEXT_VANALYZE 4

#define MENU_CHECKBOX_REDMAX 0
#define MENU_CHECKBOX_REDMIN 1
#define MENU_CHECKBOX_REDAVG 2
#define MENU_CHECKBOX_GREENMAX 3
#define MENU_CHECKBOX_GREENMIN 4
#define MENU_CHECKBOX_GREENAVG 5
#define MENU_CHECKBOX_BLUEMAX 6
#define MENU_CHECKBOX_BLUEMIN 7
#define MENU_CHECKBOX_BLUEAVG 8

#define MENU_RBUTTON_TRIGGERSOURCE 0
#define MENU_RBUTTON_TRIGGERMODE 1
#define MENU_RBUTTON_TRIGGERTYPE 2

#define MENU_LISTINDEX_ADCRED 0
#define MENU_LISTINDEX_ADCGREEN 1
#define MENU_LISTINDEX_ADCBLUE 2

#define MENU_GFX_SCOPE 0

#define MENU_GFX_SIZEX_SCOPE 320
#define MENU_GFX_SIZEY_SCOPE 160

#define MENU_GFX_FORMAT_SCOPE COLORCUSTOM1


#define MENU_USE_BOX
#define MENU_USE_LABEL
#define MENU_USE_BUTTON
#define MENU_USE_GFX
#define MENU_USE_LIST
#define MENU_USE_CHECKBOX
#define MENU_USE_RADIOBUTTON
#define MENU_USE_SUBWINDOW
#define MENU_USE_WINDOW
#define MENU_USE_SHORTCUT

#define MENU_USE_FONT_4
#define MENU_USE_FONT_5
#define MENU_USE_FONT_6
#define MENU_USE_FONT_7
#define MENU_USE_FONT_MAX 7
#define MENU_USE_UTF8
#define MENU_USE_GFXFORMAT_2

#define MENU_SDATA_3_2 926
#define MENU_SDATA_3_3 932
#define MENU_SDATA_3_4 938
#define MENU_SDATA_3_5 944
#define MENU_SDATA_3_6 950
#define MENU_SDATA_3_7 956
#define MENU_SDATA_3_25 962
#define MENU_SDATA_3_26 968
#define MENU_SDATA_3_14 974
#define MENU_SDATA_3_15 982
#define MENU_SDATA_3_16 991
#define MENU_SDATA_3_24 999
#define MENU_SDATA_3_11 1007
#define MENU_SDATA_3_12 1011
#define MENU_SDATA_3_13 1016
#define MENU_SDATA_4_1 1021
#define MENU_SDATA_4_2 1031
#define MENU_SDATA_5_1 1127
#define MENU_SDATA_5_2 1031
#define MENU_SDATA_6_1 1139
#define MENU_SDATA_6_2 1031
#define MENU_SDATA_7_13 1150
#define MENU_SDATA_7_14 1164
#define MENU_SDATA_7_15 1171
#define MENU_SDATA_7_16 1177
#define MENU_SDATA_7_17 1183
#define MENU_SDATA_7_1 1190
#define MENU_SDATA_7_2 1007
#define MENU_SDATA_7_3 1205
#define MENU_SDATA_7_4 1016
#define MENU_SDATA_7_5 1211
#define MENU_SDATA_7_6 1224
#define MENU_SDATA_7_7 1236
#define MENU_SDATA_7_8 1246
#define MENU_SDATA_7_9 1259
#define MENU_SDATA_7_10 1269
#define MENU_SDATA_7_11 1280
#define MENU_SDATA_7_12 1293
#define MENU_SDATA_8_1 1305
#define MENU_SDATA_9_13 999
#define MENU_SDATA_9_14 1311
#define MENU_SDATA_9_15 1323
#define MENU_SDATA_9_1 1339
#define MENU_SDATA_9_20 1351
#define MENU_SDATA_9_21 1356
#define MENU_SDATA_9_22 1361
#define MENU_SDATA_9_5 1366
#define MENU_SDATA_9_23 1351
#define MENU_SDATA_9_24 1356
#define MENU_SDATA_9_25 1361
#define MENU_SDATA_9_8 1380
#define MENU_SDATA_9_26 1351
#define MENU_SDATA_9_27 1356
#define MENU_SDATA_9_28 1361

#endif

