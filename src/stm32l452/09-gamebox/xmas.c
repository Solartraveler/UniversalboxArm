
#include "main.h"

#if modul_xmas

u08 xmasconfig[2] eeprom_data;


void draw_pixeladd(u08 x, u08 y, u08 color, float level) {
  if ((x < 16) && (y < 16)) {
    u08 color0 = pixel_get(x, y);
    level = fabs(level);
    if (level < 0.8) {
        pixel_set_safe(x, y, (color & 0x11) | color0);
    }
    if (level < 0.5) {
        pixel_set_safe(x, y, (color & 0x22) | color0);
    }
    if (level < 0.2) {
        pixel_set_safe(x, y, color | color0);
    }
  }
}

void draw_bresenham(u08 psx, u08 psy, u08 pex, u08 pey, u08 color) {
u08 tmp;

s08 deltax = pex - psx;
s08 deltay = pey - psy;
float error = 0;

if (abs(deltax) >= abs(deltay)) {
  if (psx > pex) {
    tmp = psx;
    psx = pex;
    pex = tmp;
    tmp = psy;
    psy = pey;
    pey = tmp;
    deltax = pex - psx;
    deltay = pey - psy;
  }

  float deltaerr = (float)deltay / (float)deltax;
  u08 y = psy;
  for (u08 x = psx; x <= pex; x++) {
    draw_pixeladd(x, y, color, error);
    draw_pixeladd(x, y+1, color, error-1.0);
    draw_pixeladd(x, y-1, color, error+1.0);
    error = error + deltaerr;
    if (error > 0.5) {
      y++;
      error = error - 1.0;
    }
    if (error < -0.5) {
      y--;
      error = error + 1.0;
    }
  }
} else {
  if (psy > pey) {
    tmp = psx;
    psx = pex;
    pex = tmp;
    tmp = psy;
    psy = pey;
    pey = tmp;
    deltax = pex - psx;
    deltay = pey - psy;
  }
  float deltaerr = (float)deltax / (float)deltay;
  u08 x = psx;
  //printf("%i %i -> %i %i %f\n", psx, psy, pex, pey, (float)deltaerr);
  for (u08 y = psy; y <= pey; y++) {
    draw_pixeladd(x, y, color, error);
    draw_pixeladd(x+1, y, color, error-1.0);
    draw_pixeladd(x-1, y, color, error+1.0);
    error = error + deltaerr;
    if (error > 0.5) {
      x++;
      error = error - 1.0;
    }
    if (error < -0.5) {
      x--;
      error = error + 1.0;
    }
  }
}
}

void draw_star(u08 px, u08 py, u08 wi, u08 wo, u08 edges, double rotate,
               u08 color) {
float polygon[edges*2][2];
u08 i;
u08 maxloop = edges*2;
float angle;
float dwi = (float)wi;
float dwo = (float)wo;
//build a star polygon
for (i = 0; i < maxloop; i++) {
  angle = (2.0*M_PI/((float)maxloop)*(float)i)+rotate;
  polygon[i][0] = sin(angle);
  polygon[i][1] = cos(angle);
  if (i & 1) {
    polygon[i][0] *= dwi;
    polygon[i][1] *= dwi;
  } else {
    polygon[i][0] *= dwo;
    polygon[i][1] *= dwo;
  }
  polygon[i][0] += (float)px + 0.5;
  polygon[i][1] += (float)py + 0.5;
}
clear_screen();
for (i = 0; i < maxloop; i++) {
  draw_bresenham(polygon[i][0], polygon[i][1],
            polygon[(i+1)%maxloop][0], polygon[(i+1)%maxloop][1], color);
}
draw_box(px-1, py-1, 3, 3, color, color);
}


void xmas_start(void) {
u08 edges;
u08 wi;
u08 changed = 1;
u16 iter = 0;
edges = eeprom_read_byte(&xmasconfig[0]);
wi = eeprom_read_byte(&xmasconfig[1]);
if (wi > 8)
  wi = 8;
if (edges > 12)
  edges = 12;
if (edges < 2)
  edges = 2;
while (userin_press() == 0) {
  if (changed) {
    draw_star(8, 8, wi, 7, edges, 0.0, 0x30);
    changed = 0;
  }
  if (iter == 1000) {
    flip_color();
    iter = 0;
    resync_led_display();
  }
  if ((userin_right()) && (edges < 12)) {
    edges++;
    changed = 1;
  }
  if ((userin_left()) && (edges > 2)) {
    edges--;
    changed = 1;
  }
  if ((userin_up()) && (wi < 8)) {
    wi++;
    changed = 1;
  }
  if ((userin_down()) && (wi > 0)) {
    wi--;
    changed = 1;
  }
  waitms(10);
  iter++;
  }
if (eeprom_read_byte(&xmasconfig[0]) != edges) {
  eeprom_write_byte(&xmasconfig[0], edges);
}
if (eeprom_read_byte(&xmasconfig[1]) != wi) {
  eeprom_write_byte(&xmasconfig[1], wi);
}
}

#endif
