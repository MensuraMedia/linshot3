#ifndef SIDEBAR_ICONS_H
#define SIDEBAR_ICONS_H

#include <cairo.h>

// Icon size - adjust this single value to resize all icons
#define SIDEBAR_ICON_SIZE 20

// Icon line weight - adjust for thicker/thinner strokes
#define SIDEBAR_ICON_STROKE 2.0

// Icon types matching sidebar button order
typedef enum {
    ICON_SHOT = 0,
    ICON_ARROW,
    ICON_BOX,
    ICON_CIRCLE,
    ICON_TEXT,
    ICON_LINE,
    ICON_SELECT,
    ICON_FLATTEN,
    ICON_COPY,
    ICON_SAVE,
    ICON_COUNT
} SidebarIconType;

// Draw an icon centered at the given position
// x, y = top-left corner of the icon bounding box
void sidebar_icon_draw(cairo_t* cr, SidebarIconType type, double x, double y);

#endif // SIDEBAR_ICONS_H
