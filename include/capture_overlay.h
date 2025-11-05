#ifndef CAPTURE_OVERLAY_H
#define CAPTURE_OVERLAY_H

#include <gtk/gtk.h>
#include <stdbool.h>
#include "screen_capture.h"

typedef struct {
    GtkWidget* window;
    GtkWidget* drawing_area;
    CaptureArea selection;
    cairo_surface_t* background;
    bool selecting;
    int start_x, start_y;
    int mouse_x, mouse_y; // Current mouse position for crosshair
} CaptureOverlay;

// Initialize and show the capture overlay
bool capture_overlay_init(CaptureOverlay* overlay);

// Get the selected area
CaptureArea capture_overlay_get_selection(CaptureOverlay* overlay);

// Clean up resources
void capture_overlay_cleanup(CaptureOverlay* overlay);

// Modular cursor management for overlay window
void set_overlay_cursor_crosshair(GtkWidget* window);
void reset_overlay_cursor(GtkWidget* window);

#endif // CAPTURE_OVERLAY_H 