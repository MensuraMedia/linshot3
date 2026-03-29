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
    settings->font.is_bold = true;              // Bold by default
    settings->font.is_italic = false;

    // Initialize shadow settings
    settings->shadow = false;
    settings->shadow_intensity = 0.4;
    settings->blur_block_size = 10;
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

// Draw a diffused multi-layer shadow behind an annotation
static void annotation_draw_shadow(Annotation* annotation, cairo_t* cr) {
    if (!annotation->settings.shadow) return;
    double intensity = annotation->settings.shadow_intensity;
    if (intensity <= 0.0) return;

    // Draw 3 shadow passes at increasing offsets with decreasing opacity for diffusion
    int passes = 3;
    for (int pass = passes; pass >= 1; pass--) {
        cairo_save(cr);
        double off = (1.0 + annotation->settings.line_width * 0.2) * pass;
        cairo_translate(cr, off, off);
        double alpha = intensity * 0.25 / pass;
        cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, alpha);
        double extra_w = 0.5 * pass;
        cairo_set_line_width(cr, annotation->settings.line_width + extra_w);

        switch (annotation->type) {
            case TOOL_ARROW: {
                // Full arrow shape shadow (shaft + head)
                double dx = annotation->bounds.x2 - annotation->bounds.x1;
                double dy = annotation->bounds.y2 - annotation->bounds.y1;
                double angle = atan2(dy, dx);
                double length = sqrt(dx * dx + dy * dy);
                if (length < 2.0) break;
                double lw = annotation->settings.line_width;
                double shaft_half = lw * 0.5 + extra_w * 0.5;
                double head_length = lw * 5.0;
                double head_half = lw * 3.0;
                if (head_length > length * 0.6) {
                    head_length = length * 0.6;
                    head_half = head_length * 0.6;
                }
                double sin_a = sin(angle), cos_a = cos(angle);
                double tip_x = annotation->bounds.x2, tip_y = annotation->bounds.y2;
                double neck_x = tip_x - head_length * cos_a;
                double neck_y = tip_y - head_length * sin_a;
                double tail_x = annotation->bounds.x1, tail_y = annotation->bounds.y1;
                double sx = sin_a * shaft_half, sy = cos_a * shaft_half;
                double hx = sin_a * head_half, hy = cos_a * head_half;
                cairo_move_to(cr, neck_x - sx, neck_y + sy);
                cairo_line_to(cr, tail_x - sx, tail_y + sy);
                cairo_arc(cr, tail_x, tail_y, shaft_half, angle + M_PI/2, angle - M_PI/2);
                cairo_line_to(cr, neck_x + sx, neck_y - sy);
                cairo_line_to(cr, neck_x + hx, neck_y - hy);
                cairo_line_to(cr, tip_x, tip_y);
                cairo_line_to(cr, neck_x - hx, neck_y + hy);
                cairo_close_path(cr);
                cairo_fill(cr);
                break;
            }
            case TOOL_LINE: {
                cairo_move_to(cr, annotation->bounds.x1, annotation->bounds.y1);
                cairo_line_to(cr, annotation->bounds.x2, annotation->bounds.y2);
                cairo_stroke(cr);
                break;
            }
            case TOOL_RECTANGLE: {
                double x = fmin(annotation->bounds.x1, annotation->bounds.x2);
                double y = fmin(annotation->bounds.y1, annotation->bounds.y2);
                double w = fabs((double)(annotation->bounds.x2 - annotation->bounds.x1));
                double h = fabs((double)(annotation->bounds.y2 - annotation->bounds.y1));
                cairo_rectangle(cr, x, y, w, h);
                cairo_stroke(cr);
                break;
            }
            case TOOL_ELLIPSE: {
                double cx2 = (annotation->bounds.x1 + annotation->bounds.x2) / 2.0;
                double cy2 = (annotation->bounds.y1 + annotation->bounds.y2) / 2.0;
                double rx = fabs((double)(annotation->bounds.x2 - annotation->bounds.x1)) / 2.0;
                double ry = fabs((double)(annotation->bounds.y2 - annotation->bounds.y1)) / 2.0;
                if (rx > 0 && ry > 0) {
                    cairo_save(cr);
                    cairo_translate(cr, cx2, cy2);
                    cairo_scale(cr, rx, ry);
                    cairo_arc(cr, 0, 0, 1.0, 0, 2 * M_PI);
                    cairo_restore(cr);
                    cairo_stroke(cr);
                }
                break;
            }
            case TOOL_TEXT: {
                if (annotation->text) {
                    PangoLayout* layout = pango_cairo_create_layout(cr);
                    PangoFontDescription* desc = pango_font_description_new();
                    pango_font_description_set_family(desc, annotation->settings.font.family ? annotation->settings.font.family : "Arial");
                    pango_font_description_set_size(desc, (int)(annotation->settings.font.size * PANGO_SCALE));
                    if (annotation->settings.font.is_bold) pango_font_description_set_weight(desc, PANGO_WEIGHT_BOLD);
                    if (annotation->settings.font.is_italic) pango_font_description_set_style(desc, PANGO_STYLE_ITALIC);
                    pango_layout_set_font_description(layout, desc);
                    pango_layout_set_text(layout, annotation->text, -1);
                    cairo_move_to(cr, annotation->bounds.x1, annotation->bounds.y1);
                    pango_cairo_show_layout(cr, layout);
                    pango_font_description_free(desc);
                    g_object_unref(layout);
                }
                break;
            }
            case TOOL_FREEHAND: {
                if (annotation->path.point_count > 1) {
                    cairo_move_to(cr, annotation->path.points[0].x1, annotation->path.points[0].y1);
                    for (int i = 1; i < annotation->path.point_count; i++) {
                        cairo_line_to(cr, annotation->path.points[i].x1, annotation->path.points[i].y1);
                    }
                    cairo_stroke(cr);
                }
                break;
            }
            case TOOL_BORDER: {
                double x = fmin(annotation->bounds.x1, annotation->bounds.x2);
                double y = fmin(annotation->bounds.y1, annotation->bounds.y2);
                double w = fabs((double)(annotation->bounds.x2 - annotation->bounds.x1));
                double h = fabs((double)(annotation->bounds.y2 - annotation->bounds.y1));
                cairo_rectangle(cr, x, y, w, h);
                cairo_stroke(cr);
                break;
            }
            default:
                break;
        }
        cairo_restore(cr);
    }
}

void annotation_draw(Annotation* annotation, cairo_t* cr) {
    if (!annotation || !cr) return;

    // Draw shadow first (behind the annotation)
    annotation_draw_shadow(annotation, cr);

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
                // Fixed-size arrow: shaft width and head size from line_width setting
                double dx = annotation->bounds.x2 - annotation->bounds.x1;
                double dy = annotation->bounds.y2 - annotation->bounds.y1;
                double angle = atan2(dy, dx);
                double length = sqrt(dx * dx + dy * dy);
                if (length < 2.0) break;

                double lw = annotation->settings.line_width;
                double shaft_half = lw * 0.5;
                double head_length = lw * 5.0;
                double head_half = lw * 3.0;

                // Clamp head to not exceed arrow length
                if (head_length > length * 0.6) {
                    head_length = length * 0.6;
                    head_half = head_length * 0.6;
                }

                double sin_a = sin(angle);
                double cos_a = cos(angle);

                double tip_x = annotation->bounds.x2;
                double tip_y = annotation->bounds.y2;
                double neck_x = tip_x - head_length * cos_a;
                double neck_y = tip_y - head_length * sin_a;
                double tail_x = annotation->bounds.x1;
                double tail_y = annotation->bounds.y1;

                double sx = sin_a * shaft_half;
                double sy = cos_a * shaft_half;
                double hx = sin_a * head_half;
                double hy = cos_a * head_half;

                // Single continuous path
                cairo_move_to(cr, neck_x - sx, neck_y + sy);
                cairo_line_to(cr, tail_x - sx, tail_y + sy);
                cairo_arc(cr, tail_x, tail_y, shaft_half,
                          angle + M_PI/2, angle - M_PI/2);
                cairo_line_to(cr, neck_x + sx, neck_y - sy);
                cairo_line_to(cr, neck_x + hx, neck_y - hy);
                cairo_line_to(cr, tip_x, tip_y);
                cairo_line_to(cr, neck_x - hx, neck_y + hy);
                cairo_close_path(cr);
                cairo_fill(cr);
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

        case TOOL_LINE:
            {
                cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
                cairo_move_to(cr, annotation->bounds.x1, annotation->bounds.y1);
                cairo_line_to(cr, annotation->bounds.x2, annotation->bounds.y2);
                cairo_stroke(cr);
            }
            break;

        case TOOL_MARQUEE:
            {
                double x = MIN(annotation->bounds.x1, annotation->bounds.x2);
                double y = MIN(annotation->bounds.y1, annotation->bounds.y2);
                double w = abs(annotation->bounds.x2 - annotation->bounds.x1);
                double h = abs(annotation->bounds.y2 - annotation->bounds.y1);

                cairo_save(cr);

                // Force sharp corners and clean state
                cairo_set_line_join(cr, CAIRO_LINE_JOIN_MITER);
                cairo_set_line_cap(cr, CAIRO_LINE_CAP_BUTT);
                cairo_set_dash(cr, NULL, 0, 0);

                // Black solid underlay for contrast
                cairo_set_line_width(cr, 2.0);
                cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.8);
                cairo_rectangle(cr, x + 0.5, y + 0.5, w, h);
                cairo_stroke(cr);

                // White dashes on top
                double dashes[] = {6.0, 4.0};
                cairo_set_dash(cr, dashes, 2, 0);
                cairo_set_line_width(cr, 1.5);
                cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 1.0);
                cairo_rectangle(cr, x + 0.5, y + 0.5, w, h);
                cairo_stroke(cr);

                cairo_restore(cr);
            }
            break;

        case TOOL_BORDER:
            {
                // Decorative border/frame around the selected region
                double x = MIN(annotation->bounds.x1, annotation->bounds.x2);
                double y = MIN(annotation->bounds.y1, annotation->bounds.y2);
                double w = abs(annotation->bounds.x2 - annotation->bounds.x1);
                double h = abs(annotation->bounds.y2 - annotation->bounds.y1);
                if (w < 2 || h < 2) break;

                double lw = annotation->settings.line_width;

                // Outer border
                cairo_set_line_width(cr, lw);
                cairo_rectangle(cr, x, y, w, h);
                cairo_stroke(cr);

                // Inner border (inset by line width * 2)
                double inset = lw * 2.5;
                if (w > inset * 2 && h > inset * 2) {
                    cairo_set_line_width(cr, lw * 0.6);
                    cairo_rectangle(cr, x + inset, y + inset, w - inset * 2, h - inset * 2);
                    cairo_stroke(cr);
                }
            }
            break;

        case TOOL_BLUR:
            {
                // Pixelate/mosaic blur effect on the selected region
                int bx = MIN(annotation->bounds.x1, annotation->bounds.x2);
                int by = MIN(annotation->bounds.y1, annotation->bounds.y2);
                int bw = abs(annotation->bounds.x2 - annotation->bounds.x1);
                int bh = abs(annotation->bounds.y2 - annotation->bounds.y1);
                if (bw < 2 || bh < 2) break;

                int block = annotation->settings.blur_block_size;
                if (block < 2) block = 2;

                // Get the target surface to read pixel data
                cairo_surface_t* target = cairo_get_target(cr);
                if (!target) break;
                cairo_surface_flush(target);

                int surf_w = cairo_image_surface_get_width(target);
                int surf_h = cairo_image_surface_get_height(target);
                unsigned char* data = cairo_image_surface_get_data(target);
                int stride = cairo_image_surface_get_stride(target);
                if (!data) break;

                // Clamp region to surface bounds
                if (bx < 0) bx = 0;
                if (by < 0) by = 0;
                if (bx + bw > surf_w) bw = surf_w - bx;
                if (by + bh > surf_h) bh = surf_h - by;

                // For each block, average the pixel colors and fill
                for (int row = by; row < by + bh; row += block) {
                    for (int col = bx; col < bx + bw; col += block) {
                        int blk_w = (col + block <= bx + bw) ? block : (bx + bw - col);
                        int blk_h = (row + block <= by + bh) ? block : (by + bh - row);

                        // Average colors in this block
                        long r_sum = 0, g_sum = 0, b_sum = 0;
                        int count = 0;
                        for (int py = row; py < row + blk_h && py < surf_h; py++) {
                            for (int px = col; px < col + blk_w && px < surf_w; px++) {
                                int idx = py * stride + px * 4;
                                b_sum += data[idx + 0];
                                g_sum += data[idx + 1];
                                r_sum += data[idx + 2];
                                count++;
                            }
                        }
                        if (count > 0) {
                            double r = (double)(r_sum / count) / 255.0;
                            double g = (double)(g_sum / count) / 255.0;
                            double b = (double)(b_sum / count) / 255.0;
                            cairo_set_source_rgb(cr, r, g, b);
                            cairo_rectangle(cr, col, row, blk_w, blk_h);
                            cairo_fill(cr);
                        }
                    }
                }
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