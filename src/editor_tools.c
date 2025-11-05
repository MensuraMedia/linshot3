#include "../include/editor_tools.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <pango/pango.h>
#include <pango/pangocairo.h>

void tool_settings_init(ToolSettings* settings) {
    if (!settings) return;
    
    settings->type = TOOL_NONE;
    settings->color.red = 1.0;     // Red by default
    settings->color.green = 0.0;
    settings->color.blue = 0.0;
    settings->color.alpha = 1.0;
    settings->line_width = 2.0;
    settings->fill = false;
    
    // Initialize font settings
    settings->font.family = g_strdup("Arial");  // Default to Arial
    settings->font.size = 14.0;                 // 14pt by default
    settings->font.is_bold = false;
    settings->font.is_italic = false;
}

Annotation* annotation_create(ToolType type, ToolSettings* settings) {
    if (!settings) return NULL;
    
    Annotation* annotation = (Annotation*)malloc(sizeof(Annotation));
    if (!annotation) return NULL;
    
    memset(annotation, 0, sizeof(Annotation));
    annotation->type = type;
    
    // Copy the settings
    annotation->settings = *settings;
    if (settings->font.family) {
        annotation->settings.font.family = g_strdup(settings->font.family);
    }
    
    return annotation;
}

void annotation_draw(Annotation* annotation, cairo_t* cr) {
    if (!annotation || !cr) return;
    
    // Set up common drawing properties
    cairo_set_source_rgba(cr,
                         annotation->settings.color.red,
                         annotation->settings.color.green,
                         annotation->settings.color.blue,
                         annotation->settings.color.alpha);
    cairo_set_line_width(cr, annotation->settings.line_width);
    
    switch (annotation->type) {
        case TOOL_ARROW:
            {
                // Calculate arrow direction
                double dx = annotation->bounds.x2 - annotation->bounds.x1;
                double dy = annotation->bounds.y2 - annotation->bounds.y1;
                double angle = atan2(dy, dx);
                double length = sqrt(dx * dx + dy * dy);
                
                // Draw arrow shaft with 3pt width
                cairo_set_line_width(cr, 3.0);
                cairo_move_to(cr, annotation->bounds.x1, annotation->bounds.y1);
                cairo_line_to(cr, annotation->bounds.x2 - 12.0 * cos(angle), annotation->bounds.y2 - 12.0 * sin(angle));
                cairo_stroke(cr);
                
                // Draw arrow head
                // Make arrow head size proportional to shaft length, but with min/max limits
                double arrow_length = fmin(fmax(length * 0.15, 12.0), 20.0);  // 15% of shaft length, between 12-20px
                double arrow_width = arrow_length * 0.8;  // Width is 80% of length for good proportions
                
                // Calculate arrow head points
                double tip_x = annotation->bounds.x2;
                double tip_y = annotation->bounds.y2;
                double back_x = tip_x - arrow_length * cos(angle);
                double back_y = tip_y - arrow_length * sin(angle);
                
                // Points for the base of the arrow head
                double left_x = back_x - (arrow_width * 0.5) * sin(angle);
                double left_y = back_y + (arrow_width * 0.5) * cos(angle);
                double right_x = back_x + (arrow_width * 0.5) * sin(angle);
                double right_y = back_y - (arrow_width * 0.5) * cos(angle);
                
                // Draw solid arrow head with slightly thicker outline
                cairo_set_line_width(cr, 1.0);
                cairo_move_to(cr, tip_x, tip_y);
                cairo_line_to(cr, left_x, left_y);
                cairo_line_to(cr, right_x, right_y);
                cairo_close_path(cr);
                cairo_fill_preserve(cr);
                cairo_stroke(cr);
            }
            break;
            
        case TOOL_RECTANGLE:
            {
                double width = abs(annotation->bounds.x2 - annotation->bounds.x1);
                double height = abs(annotation->bounds.y2 - annotation->bounds.y1);
                double x = MIN(annotation->bounds.x1, annotation->bounds.x2);
                double y = MIN(annotation->bounds.y1, annotation->bounds.y2);
                cairo_rectangle(cr, x, y, width, height);
                if (annotation->settings.fill) {
                    cairo_fill_preserve(cr);
                }
                cairo_stroke(cr);
            }
            break;
            
        case TOOL_ELLIPSE:
            {
                double width = abs(annotation->bounds.x2 - annotation->bounds.x1);
                double height = abs(annotation->bounds.y2 - annotation->bounds.y1);
                double x = MIN(annotation->bounds.x1, annotation->bounds.x2);
                double y = MIN(annotation->bounds.y1, annotation->bounds.y2);
                
                // Save current transformation
                cairo_save(cr);
                
                // Translate to center of ellipse
                cairo_translate(cr, x + width/2, y + height/2);
                
                // Scale to make a circle into an ellipse
                cairo_scale(cr, width/2, height/2);
                
                // Draw a circle that will be scaled into an ellipse
                cairo_arc(cr, 0, 0, 1, 0, 2 * M_PI);
                
                // Restore original transformation
                cairo_restore(cr);
                
                if (annotation->settings.fill) {
                    cairo_fill_preserve(cr);
                }
                cairo_stroke(cr);
            }
            break;
            
        case TOOL_TEXT:
            if (annotation->text) {
                // Create Pango layout for better text rendering
                PangoLayout* layout = pango_cairo_create_layout(cr);
                PangoFontDescription* desc = pango_font_description_new();
                
                // Set font family
                const char* family = annotation->settings.font.family ? 
                                   annotation->settings.font.family : "Arial";
                pango_font_description_set_family(desc, family);
                
                // Set font size (convert points to Pango units)
                double size_pts = annotation->settings.font.size;
                pango_font_description_set_size(desc, (int)(size_pts * PANGO_SCALE));
                
                // Set font weight
                PangoWeight weight = annotation->settings.font.is_bold ? 
                                   PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL;
                pango_font_description_set_weight(desc, weight);
                
                // Set font style
                PangoStyle style = annotation->settings.font.is_italic ? 
                                 PANGO_STYLE_ITALIC : PANGO_STYLE_NORMAL;
                pango_font_description_set_style(desc, style);
                
                // Apply font description to layout
                pango_layout_set_font_description(layout, desc);
                pango_font_description_free(desc);
                
                // Set text and get its size
                pango_layout_set_text(layout, annotation->text, -1);
                PangoRectangle ink_rect, logical_rect;
                pango_layout_get_extents(layout, &ink_rect, &logical_rect);
                
                // Convert Pango units to Cairo units
                double text_width = (double)logical_rect.width / PANGO_SCALE;
                double text_height = (double)logical_rect.height / PANGO_SCALE;
                
                // Store text dimensions for hit testing
                annotation->bounds.x2 = annotation->bounds.x1 + text_width;
                annotation->bounds.y2 = annotation->bounds.y1 + text_height;
                
                // Draw text
                cairo_move_to(cr, annotation->bounds.x1, annotation->bounds.y1);
                pango_cairo_show_layout(cr, layout);
                
                // Clean up
                g_object_unref(layout);
            }
            break;
            
        case TOOL_FREEHAND:
            if (annotation->path.point_count > 0) {
                cairo_move_to(cr, annotation->path.points[0].x1, annotation->path.points[0].y1);
                for (int i = 1; i < annotation->path.point_count; i++) {
                    cairo_line_to(cr, annotation->path.points[i].x1, annotation->path.points[i].y1);
                }
                cairo_stroke(cr);
            }
            break;
            
        default:
            break;
    }
}

void annotation_free(Annotation* annotation) {
    if (!annotation) return;
    
    if (annotation->text) {
        g_free(annotation->text);
        annotation->text = NULL;
    }
    
    if (annotation->settings.font.family) {
        g_free(annotation->settings.font.family);
        annotation->settings.font.family = NULL;
    }
    
    free(annotation);
} 