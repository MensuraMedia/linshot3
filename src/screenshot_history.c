#include "../include/screenshot_history.h"
#include "../include/main_window.h"
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>

#define THUMBNAIL_SIZE 200

static int compare_entries_by_time(gconstpointer a, gconstpointer b) {
    const ScreenshotEntry* entry_a = a;
    const ScreenshotEntry* entry_b = b;
    return (entry_b->timestamp - entry_a->timestamp);  // Most recent first
}

static GdkPixbuf* create_thumbnail(const char* filepath) {
    GdkPixbuf* original = gdk_pixbuf_new_from_file(filepath, NULL);
    if (!original) return NULL;
    
    int width = gdk_pixbuf_get_width(original);
    int height = gdk_pixbuf_get_height(original);
    
    // Calculate thumbnail dimensions maintaining aspect ratio
    double scale_w = (double)THUMBNAIL_SIZE / width;
    double scale_h = (double)THUMBNAIL_SIZE / height;
    double scale = MIN(scale_w, scale_h);  // Use the smaller scale to fit within bounds
    
    int thumb_width = width * scale;
    int thumb_height = height * scale;
    
    // Create a new pixbuf with transparent background
    GdkPixbuf* background = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, THUMBNAIL_SIZE, THUMBNAIL_SIZE);
    gdk_pixbuf_fill(background, 0x00000000);  // Transparent background (alpha = 0)
    
    // Scale the original image
    GdkPixbuf* scaled = gdk_pixbuf_scale_simple(original, 
                                               thumb_width, 
                                               thumb_height, 
                                               GDK_INTERP_BILINEAR);
    
    // Center the scaled image on the background
    int x_offset = (THUMBNAIL_SIZE - thumb_width) / 2;
    int y_offset = (THUMBNAIL_SIZE - thumb_height) / 2;
    
    // Copy the scaled image onto the background, preserving alpha
    gdk_pixbuf_copy_area(scaled, 0, 0, thumb_width, thumb_height,
                        background, x_offset, y_offset);
    
    // Clean up
    g_object_unref(original);
    g_object_unref(scaled);
    
    return background;
}

static void screenshot_entry_free(ScreenshotEntry* entry) {
    if (!entry) return;
    g_free(entry->filepath);
    if (entry->thumbnail) g_object_unref(entry->thumbnail);
    g_free(entry);
}

void screenshot_history_init(ScreenshotHistory* history) {
    history->entries = NULL;
    history->screenshot_path = g_strdup(g_get_user_special_dir(G_USER_DIRECTORY_PICTURES));
}

void screenshot_history_set_path(ScreenshotHistory* history, const char* path) {
    if (!history) return;
    g_free(history->screenshot_path);
    history->screenshot_path = g_strdup(path);
}

void screenshot_history_add(ScreenshotHistory* history, const char* filepath) {
    if (!history || !filepath) return;
    
    // Check if file exists and is readable
    if (access(filepath, R_OK) != 0) return;
    
    // Get file timestamp
    struct stat st;
    if (stat(filepath, &st) != 0) return;
    
    // Create new entry
    ScreenshotEntry* entry = g_new0(ScreenshotEntry, 1);
    entry->filepath = g_strdup(filepath);
    entry->timestamp = st.st_mtime;
    entry->thumbnail = create_thumbnail(filepath);
    
    if (!entry->thumbnail) {
        screenshot_entry_free(entry);
        return;
    }
    
    // Add to list
    history->entries = g_list_insert_sorted(history->entries, 
                                          entry, 
                                          compare_entries_by_time);
}

void screenshot_history_load(ScreenshotHistory* history) {
    if (!history) return;
    
    // Clean up existing entries first
    g_list_free_full(history->entries, (GDestroyNotify)screenshot_entry_free);
    history->entries = NULL;
    
    // Get the screenshot directory from settings
    const char* screenshot_dir = history->screenshot_path;
    if (!screenshot_dir) {
        // Fallback to Downloads directory if no path is set
        screenshot_dir = g_get_user_special_dir(G_USER_DIRECTORY_DOWNLOAD);
        if (!screenshot_dir) return;
    }
    
    DIR* dir = opendir(screenshot_dir);
    if (!dir) return;
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        // Skip . and ..
        if (entry->d_name[0] == '.') continue;
        
        // Check if filename starts with "LinShot" or "Screenshot"
        if (strncmp(entry->d_name, "LinShot", 7) == 0 || 
            strncmp(entry->d_name, "Screenshot", 9) == 0) {
            char* filepath = g_build_filename(screenshot_dir, entry->d_name, NULL);
            screenshot_history_add(history, filepath);
            g_free(filepath);
        }
    }
    
    closedir(dir);
}

void screenshot_history_cleanup(ScreenshotHistory* history) {
    if (!history) return;
    g_list_free_full(history->entries, (GDestroyNotify)screenshot_entry_free);
    history->entries = NULL;
    g_free(history->screenshot_path);
    history->screenshot_path = NULL;
}

GList* screenshot_history_get_sorted(ScreenshotHistory* history) {
    if (!history) return NULL;
    return history->entries;  // Already sorted when adding entries
} 