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

uint8_t volatile gdata[screeny][screenx];

int windowid_main;

int redraw_ms = 100; //Alle x ms neu Zeichnen. Bei > 1 CPU -> 16ms erhöht

void resync_led_display(void) {
}

float colortranslater[4] = {0.0,0.8,0.9,1.0};
float colortranslateg[4] = {0.0,0.5,0.65,0.8};

static void drawboard(void) {
u08 px,py;
u08 color,red,green;
float redf,greenf,bluef;
glClear(GL_COLOR_BUFFER_BIT);
for (py = 0; py < screeny; py++) {
  for (px = 0; px < screenx; px++) {
    glPushMatrix();
    color = gdata[15-py][px];
    red = color & 0x03;
    green = (color & 0x30)>>4;
    redf = colortranslater[red];
    greenf = colortranslateg[green];
    bluef = 0.16; //um die Farben aufzuhelllen
    glColor3f(redf,greenf,bluef);
    glTranslatef((px-8.0)/9.0+0.05,(py-8.0)/9.0+0.05,0.0);
    glutSolidSphere(0.05, 20, 10);
    glPopMatrix();
  }
}
glutSwapBuffers();
glFlush();
}

static void redraw(int param) {
glutPostRedisplay();
glutTimerFunc(redraw_ms,redraw, 0); //10FPS
}

static void update_window_size(int width, int height) {
drawboard();
}

void init_window(void) {
long cpus = sysconf(_SC_NPROCESSORS_CONF);
if (cpus > 1) {
  redraw_ms = 16;
  printf("Info: Using refresh rate for multi CPU systems\n");
}
glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);
glutInitWindowSize(600,600);
glutInitWindowPosition(100,20);
windowid_main = glutCreateWindow("Game Box 1.02 (Final)");
glutDisplayFunc(drawboard);    //Zeichnet das Spielfeld neu bei überlappen o.ä.
glutReshapeFunc(update_window_size);
glutTimerFunc(redraw_ms,redraw, 0); //20FPS, Periodisches neuzeichnen
glClearColor(0.0,0.0,0.0,0.0); //"Farbe" zum löschen
}
