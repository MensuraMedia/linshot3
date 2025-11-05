#ifndef CROSSHAIR_DRAWER_H
#define CROSSHAIR_DRAWER_H

#include <cairo.h>
#include <gdk/gdk.h>

// Draw a crosshair at (x, y) with the given size and color
void crosshair_drawer_draw(cairo_t *cr, int x, int y, int size, GdkRGBA *color);

#endif // CROSSHAIR_DRAWER_H 