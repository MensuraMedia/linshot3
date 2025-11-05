#ifndef SCREEN_CAPTURE_H
#define SCREEN_CAPTURE_H

#include <stdbool.h>
#include <stdint.h>
#include <cairo/cairo.h>

typedef struct {
    int x, y;
    int width, height;
} CaptureArea;

typedef enum {
    CAPTURE_FULLSCREEN,
    CAPTURE_AREA,
    CAPTURE_WINDOW
} CaptureMode;

// Initialize screen capture
bool capture_init(void);

// Capture screen based on mode and area
cairo_surface_t* capture_screen(CaptureMode mode, CaptureArea* area);

// Clean up resources
void capture_cleanup(void);

#endif // SCREEN_CAPTURE_H 