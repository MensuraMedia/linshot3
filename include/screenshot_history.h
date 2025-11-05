#ifndef SCREENSHOT_HISTORY_H
#define SCREENSHOT_HISTORY_H

#include <gtk/gtk.h>
#include <stdbool.h>
#include <time.h>

typedef struct {
    char* filepath;
    time_t timestamp;
    GdkPixbuf* thumbnail;
} ScreenshotEntry;

typedef struct {
    GList* entries;  // List of ScreenshotEntry
    char* screenshot_path;  // Path where screenshots are stored
} ScreenshotHistory;

// Initialize screenshot history
void screenshot_history_init(ScreenshotHistory* history);

// Add a new screenshot to history
void screenshot_history_add(ScreenshotHistory* history, const char* filepath);

// Load existing screenshots from disk
void screenshot_history_load(ScreenshotHistory* history);

// Clean up screenshot history
void screenshot_history_cleanup(ScreenshotHistory* history);

// Get sorted list of screenshots (most recent first)
GList* screenshot_history_get_sorted(ScreenshotHistory* history);

// Set the screenshot path
void screenshot_history_set_path(ScreenshotHistory* history, const char* path);

#endif // SCREENSHOT_HISTORY_H 