/*
   Gamebox
    Copyright (C) 2004-2006  by Malte Marwedel
    m.marwedel AT onlinehome dot de

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "main.h"


struct userinputstruct userin;
struct userinputcalibstruct calib_x;
struct userinputcalibstruct calib_y;

u08 volatile snap_x;
u08 volatile snap_y;

#if modul_calib_save
void calib_load(void) {

}

void calib_save(void) {

}

#endif

void input_calib(void) {
}

void input_select(void) {

}

void input_key_key (unsigned char key, int x, int y) {
if (key == ' ') {
  userin.press = 1;
}
if (key == 'q') {  //Programm beenden
  exit(0);
}
}

void input_key_cursor (int key, int x, int y) {
if (key == GLUT_KEY_LEFT) {
  userin.left = 1;
}
if (key == GLUT_KEY_RIGHT) {
  userin.right = 1;
}
if (key == GLUT_KEY_UP) {
  userin.up = 1;
}
if (key == GLUT_KEY_DOWN) {
  userin.down = 1;
}
}

void input_mouse_key(int button, int state, int x, int y) {
if ((state == GLUT_UP) && (button == GLUT_LEFT_BUTTON)) {
  userin.press = 1;
}
}

void input_mouse_move(int x, int y) {
s16 temp;
temp =  (x-300)/2.3;
if (temp < -127) {
  temp = -127;
}
if (temp > 127) {
  temp = 127;
}
userin.x = temp;
temp =  (y-300)/2.3;
if (temp < -127) {
  temp = -127;
}
if (temp > 127) {
  temp = 127;
}
userin.y = temp;
//right
if ((userin.x > 100) && (snap_x == 0)) {
  userin.right = 1;
  snap_x = 1;
}
if ((userin.x < 90) && (userin.x > 0)) {
  snap_x = 0;
}
//left
if ((userin.x < -100) && (snap_x == 0)) {
  userin.left = 1;
  snap_x = 1;
}
if ((userin.x > -90) && (userin.x < 0)) {
  snap_x = 0;
}
//down
if ((userin.y > 100) && (snap_y == 0)) {
  userin.down = 1;
  snap_y = 1;
}
if ((userin.y < 90) && (userin.y > 0)) {
  snap_y = 0;
}
//up
if ((userin.y < -100) && (snap_y == 0)) {
  userin.up = 1;
  snap_y = 1;
}
if ((userin.y > -90) && (userin.y < 0)) {
  snap_y = 0;
}
}

void reduceCPU(void) {
static int calls = 0;
if (calls == 10) {
  usleep(1000);
  calls = 0;
}
calls++;
}

u08 userin_left(void) {
reduceCPU();
if (userin.left) {
  userin.left = 0;
  return 1;
}
return 0;
}

u08 userin_right(void) {
reduceCPU();
if (userin.right) {
  userin.right = 0;
  return 1;
}
return 0;
}

u08 userin_up(void) {
reduceCPU();
if (userin.up) {
  userin.up = 0;
  return 1;
}
return 0;
}

u08 userin_down(void) {
reduceCPU();
if (userin.down) {
  userin.down = 0;
  return 1;
}
return 0;
}

u08 userin_press(void) {
reduceCPU();
if (userin.press) {
  userin.press = 0;
  return 1;
}
return 0;
}

void userin_flush(void) {
userin.left = 0;
userin.right = 0;
userin.up = 0;
userin.down = 0;
userin.press = 0;
}
