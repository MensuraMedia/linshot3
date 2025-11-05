#include "../include/screen_capture.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <cairo/cairo-xlib.h>
#include <stdlib.h>
#include <stdio.h>

static Display* display = NULL;
static Window root = 0;

bool capture_init(void) {
    display = XOpenDisplay(NULL);
    if (!display) {
        fprintf(stderr, "Unable to open X display\n");
        return false;
    }
    
    root = DefaultRootWindow(display);
    return true;
}

cairo_surface_t* capture_screen(CaptureMode mode, CaptureArea* area) {
    XWindowAttributes wattr;
    int x = 0, y = 0;
    int width, height;
    
    if (mode == CAPTURE_FULLSCREEN) {
        XGetWindowAttributes(display, root, &wattr);
        width = wattr.width;
        height = wattr.height;
    } else if (mode == CAPTURE_AREA && area != NULL) {
        x = area->x;
        y = area->y;
        width = area->width;
        height = area->height;
    } else {
        return NULL;
    }
    
    XImage* img = XGetImage(display, root, x, y, width, height, AllPlanes, ZPixmap);
    if (!img) {
        fprintf(stderr, "Unable to get image from display\n");
        return NULL;
    }
    
    // Create a Cairo surface from the X image
    cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
    if (cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS) {
        fprintf(stderr, "Unable to create Cairo surface\n");
        XDestroyImage(img);
        return NULL;
    }
    
    // Copy the X image data to the Cairo surface
    unsigned char* cairo_data = cairo_image_surface_get_data(surface);
    int cairo_stride = cairo_image_surface_get_stride(surface);
    
    // Calculate bit shifts for each color component
    int red_shift = 0, green_shift = 0, blue_shift = 0;
    unsigned long mask;
    
    // Calculate red shift
    mask = img->red_mask;
    while (mask && !(mask & 1)) {
        mask >>= 1;
        red_shift++;
    }
    
    // Calculate green shift
    mask = img->green_mask;
    while (mask && !(mask & 1)) {
        mask >>= 1;
        green_shift++;
    }
    
    // Calculate blue shift
    mask = img->blue_mask;
    while (mask && !(mask & 1)) {
        mask >>= 1;
        blue_shift++;
    }
    
    for (int j = 0; j < height; j++) {
        for (int i = 0; i < width; i++) {
            unsigned long pixel = XGetPixel(img, i, j);
            unsigned char* p = cairo_data + j * cairo_stride + i * 4;
            
            // Extract color components using the correct masks and shifts
            p[0] = (pixel & img->blue_mask) >> blue_shift;     // Blue
            p[1] = (pixel & img->green_mask) >> green_shift;   // Green
            p[2] = (pixel & img->red_mask) >> red_shift;       // Red
            p[3] = 0xFF;                                       // Alpha (fully opaque)
        }
    }
    
    cairo_surface_mark_dirty(surface);
    XDestroyImage(img);
    
    return surface;
}

void capture_cleanup(void) {
    if (display) {
        XCloseDisplay(display);
        display = NULL;
    }
} 