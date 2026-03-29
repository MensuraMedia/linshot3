#include "sidebar_icons.h"
#include <math.h>

// All coordinates are normalized to a 20x20 grid.
// If SIDEBAR_ICON_SIZE changes, icons scale proportionally.

static double S(double val) {
    return val * ((double)SIDEBAR_ICON_SIZE / 20.0);
}

// SET 1 design (Clean Minimal shapes) with SET 2 styling (bold strokes)
// Save icon uses SET 2 design (download arrow into tray)

static void draw_shot(cairo_t* cr, double x, double y) {
    double lw = SIDEBAR_ICON_STROKE;
    cairo_set_line_width(cr, lw);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_BUTT);
    cairo_set_line_join(cr, CAIRO_LINE_JOIN_MITER);

    // Camera body
    cairo_rectangle(cr, x+S(2), y+S(5), S(16), S(12));
    cairo_stroke(cr);

    // Lens circle
    cairo_arc(cr, x+S(10), y+S(11), S(3.5), 0, 2*M_PI);
    cairo_stroke(cr);

    // Flash bump
    cairo_move_to(cr, x+S(7), y+S(5));
    cairo_line_to(cr, x+S(8), y+S(2));
    cairo_line_to(cr, x+S(12), y+S(2));
    cairo_line_to(cr, x+S(13), y+S(5));
    cairo_stroke(cr);
}

static void draw_arrow(cairo_t* cr, double x, double y) {
    double lw = SIDEBAR_ICON_STROKE;
    cairo_set_line_width(cr, lw);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_BUTT);
    cairo_set_line_join(cr, CAIRO_LINE_JOIN_MITER);

    // Shaft
    cairo_move_to(cr, x+S(4), y+S(16));
    cairo_line_to(cr, x+S(16), y+S(4));
    cairo_stroke(cr);

    // Arrowhead lines
    cairo_move_to(cr, x+S(16), y+S(4));
    cairo_line_to(cr, x+S(10), y+S(4));
    cairo_stroke(cr);
    cairo_move_to(cr, x+S(16), y+S(4));
    cairo_line_to(cr, x+S(16), y+S(10));
    cairo_stroke(cr);
}

static void draw_box(cairo_t* cr, double x, double y) {
    double lw = SIDEBAR_ICON_STROKE;
    cairo_set_line_width(cr, lw);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_BUTT);
    cairo_set_line_join(cr, CAIRO_LINE_JOIN_MITER);

    cairo_rectangle(cr, x+S(3), y+S(3), S(14), S(14));
    cairo_stroke(cr);
}

static void draw_circle(cairo_t* cr, double x, double y) {
    double lw = SIDEBAR_ICON_STROKE;
    cairo_set_line_width(cr, lw);

    cairo_arc(cr, x+S(10), y+S(10), S(7), 0, 2*M_PI);
    cairo_stroke(cr);
}

static void draw_text(cairo_t* cr, double x, double y) {
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_BUTT);
    cairo_set_line_join(cr, CAIRO_LINE_JOIN_MITER);

    // Top bar of T
    cairo_set_line_width(cr, SIDEBAR_ICON_STROKE);
    cairo_move_to(cr, x+S(4), y+S(4));
    cairo_line_to(cr, x+S(16), y+S(4));
    cairo_stroke(cr);

    // Vertical stroke (slightly heavier)
    cairo_set_line_width(cr, SIDEBAR_ICON_STROKE + 0.4);
    cairo_move_to(cr, x+S(10), y+S(4));
    cairo_line_to(cr, x+S(10), y+S(17));
    cairo_stroke(cr);

    // Bottom serif
    cairo_set_line_width(cr, SIDEBAR_ICON_STROKE);
    cairo_move_to(cr, x+S(7), y+S(17));
    cairo_line_to(cr, x+S(13), y+S(17));
    cairo_stroke(cr);
}

static void draw_copy(cairo_t* cr, double x, double y) {
    double lw = SIDEBAR_ICON_STROKE;
    cairo_set_line_width(cr, lw);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_BUTT);
    cairo_set_line_join(cr, CAIRO_LINE_JOIN_MITER);

    // Back rectangle
    cairo_rectangle(cr, x+S(2), y+S(2), S(11), S(13));
    cairo_stroke(cr);

    // Front rectangle (offset)
    cairo_rectangle(cr, x+S(7), y+S(5), S(11), S(13));
    cairo_stroke(cr);
}

static void draw_save(cairo_t* cr, double x, double y) {
    // SET 2 save icon: download arrow into tray
    double lw = SIDEBAR_ICON_STROKE;
    cairo_set_line_width(cr, lw);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_BUTT);
    cairo_set_line_join(cr, CAIRO_LINE_JOIN_MITER);

    // Downward arrow shaft
    cairo_move_to(cr, x+S(10), y+S(3));
    cairo_line_to(cr, x+S(10), y+S(13));
    cairo_stroke(cr);

    // Arrow chevron
    cairo_move_to(cr, x+S(6), y+S(10));
    cairo_line_to(cr, x+S(10), y+S(14));
    cairo_line_to(cr, x+S(14), y+S(10));
    cairo_stroke(cr);

    // Tray
    cairo_move_to(cr, x+S(3), y+S(14));
    cairo_line_to(cr, x+S(3), y+S(18));
    cairo_line_to(cr, x+S(17), y+S(18));
    cairo_line_to(cr, x+S(17), y+S(14));
    cairo_stroke(cr);
}

// Dispatch table
typedef void (*IconDrawFunc)(cairo_t*, double, double);

static const IconDrawFunc icon_funcs[ICON_COUNT] = {
    draw_shot,
    draw_arrow,
    draw_box,
    draw_circle,
    draw_text,
    draw_copy,
    draw_save
};

void sidebar_icon_draw(cairo_t* cr, SidebarIconType type, double x, double y) {
    if (type < 0 || type >= ICON_COUNT) return;

    // Save state so icon drawing doesn't affect caller
    cairo_save(cr);
    icon_funcs[type](cr, x, y);
    cairo_restore(cr);
}
