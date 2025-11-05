#include "../include/crosshair_drawer.h"

void crosshair_drawer_draw(cairo_t *cr, int x, int y, int size, GdkRGBA *color) {
    if (!cr || !color) return;
    cairo_save(cr);
    cairo_set_source_rgba(cr, color->red, color->green, color->blue, color->alpha);
    cairo_set_line_width(cr, 2.0);
    // Horizontal line
    cairo_move_to(cr, x - size, y);
    cairo_line_to(cr, x + size, y);
    // Vertical line
    cairo_move_to(cr, x, y - size);
    cairo_line_to(cr, x, y + size);
    cairo_stroke(cr);
    cairo_restore(cr);
} 