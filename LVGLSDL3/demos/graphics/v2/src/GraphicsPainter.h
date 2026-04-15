#pragma once

#include "GraphicsGeom.h"
#include <format>
#include <string>

namespace graphics::v2
{

    struct GraphicsPen {
        int width        = 1;
        lv_color_t color = lv_color_make(0x00, 0x00, 0x00);
    };

    struct GraphicsBrush {
        lv_color_t color = lv_color_make(0xff, 0xff, 0xff);
    };

    class GraphicsPainter
    {
    public:
        GraphicsPainter(lv_event_t* e) {
            m_event    = e;
            m_obj      = lv_event_get_target_obj(e);
            m_layer    = lv_event_get_layer(e);

            m_offset_x = lv_obj_get_x(m_obj);
            m_offset_y = lv_obj_get_y(m_obj);
        }

        lv_event_t* get_event() { return m_event; }

        lv_layer_t* get_layer() { return m_layer; }

        void set_pen_width(int width) { m_pen.width = width; }

        void set_pen_color(lv_color_t color) { m_pen.color = color; }

        void set_brush_color(lv_color_t color) { m_brush.color = color; }

        void draw_point(geom::point_t<> point) {}

        void draw_line(geom::line_t<> line) {
            lv_draw_line_dsc_t dsc;
            lv_draw_line_dsc_init(&dsc);
            dsc.width = m_pen.width;
            dsc.color = m_pen.color;

            dsc.p1    = {line.start.x, line.start.y};
            dsc.p2    = {line.end.x, line.end.y};

            dsc.p1.x += m_offset_x;
            dsc.p1.y += m_offset_y;
            dsc.p2.x += m_offset_x;
            dsc.p2.y += m_offset_y;

            lv_draw_line(m_layer, &dsc);
        }

        void draw_polyline(geom::polyline_t<> polyline) {
            lv_draw_line_dsc_t dsc;
            lv_draw_line_dsc_init(&dsc);
            dsc.width = m_pen.width;
            dsc.color = m_pen.color;

            for (auto& v : polyline.points) {
                v.x += m_offset_x;
                v.y += m_offset_y;
            }

            dsc.points    = reinterpret_cast<lv_point_precise_t*>(polyline.points.data());
            dsc.point_cnt = polyline.points.size();
            lv_draw_line(m_layer, &dsc);
        }

        void draw_arc(geom::arc_t<> arc) {
            lv_draw_arc_dsc_t dsc;
            lv_draw_arc_dsc_init(&dsc);
            dsc.color    = m_pen.color;
            dsc.width    = m_pen.width;
            dsc.opa      = LV_OPA_COVER;
            dsc.center.x = arc.radius;
            dsc.center.y = arc.radius;

            dsc.center.x += m_offset_x;
            dsc.center.y += m_offset_y;

            dsc.radius      = arc.radius;
            dsc.start_angle = arc.start_angle;
            dsc.end_angle   = arc.end_angle;
            lv_draw_arc(m_layer, &dsc);
        }

        void draw_triangle(geom::triangle_t<> triangle) {
            geom::polyline_t<> polyline;
            polyline.points.emplace_back(triangle.a);
            polyline.points.emplace_back(triangle.b);
            polyline.points.emplace_back(triangle.c);
            polyline.points.emplace_back(triangle.a);
            draw_polyline(polyline);
        }

        void draw_circle(geom::circle_t<> circle) {
            lv_draw_arc_dsc_t dsc;
            lv_draw_arc_dsc_init(&dsc);
            dsc.color    = m_pen.color;
            dsc.width    = m_pen.width;
            dsc.opa      = LV_OPA_COVER;
            dsc.center.x = circle.radius;
            dsc.center.y = circle.radius;

            dsc.center.x += m_offset_x;
            dsc.center.y += m_offset_y;

            dsc.radius      = circle.radius;
            dsc.start_angle = 0;
            dsc.end_angle   = 360;
            lv_draw_arc(m_layer, &dsc);
        }

        void draw_rect(geom::rect_t<> rect) {
            lv_draw_rect_dsc_t dsc;
            lv_draw_rect_dsc_init(&dsc);

            dsc.bg_opa       = LV_OPA_TRANSP;
            dsc.border_color = m_pen.color;
            dsc.border_width = m_pen.width;

            lv_area_t area;
            lv_obj_get_coords(m_obj, &area);
            lv_draw_rect(m_layer, &dsc, &area);
        }

        void draw_polygon(geom::polygon_t<> polygon) {
            lv_draw_line_dsc_t dsc;
            lv_draw_line_dsc_init(&dsc);
            dsc.width = m_pen.width;
            dsc.color = m_pen.color;

            for (auto& v : polygon.vertices) {
                v.x += m_offset_x;
                v.y += m_offset_y;
            }

            dsc.points    = reinterpret_cast<lv_point_precise_t*>(polygon.vertices.data());
            dsc.point_cnt = polygon.vertices.size();
            lv_draw_line(m_layer, &dsc);
        }

        void draw_curve(geom::curve_t<> curve, std::vector<lv_fpoint_t> pts) {
            lv_draw_vector_dsc_t* ctx = lv_draw_vector_dsc_create(m_layer);
            lv_vector_path_t* path    = lv_vector_path_create(LV_VECTOR_PATH_QUALITY_MEDIUM);
            lv_vector_path_clear(path);
            lv_draw_vector_dsc_identity(ctx);

            for (int i = 0; i < sizeof(pts) / sizeof(lv_fpoint_t); i++) {
                pts[i].x += m_offset_x;
                pts[i].y += m_offset_y;
            }

            auto draw_handle = [&](lv_fpoint_t const& c, float r = 6)
            {
                lv_vector_path_move_to(path, &c);
                lv_vector_path_append_circle(path, &c, r, r);
            };

            draw_handle(pts[0]);
            draw_handle(pts[1]);
            draw_handle(pts[2]);
            draw_handle(pts[3]);

            lv_draw_vector_dsc_set_stroke_opa(ctx, LV_OPA_TRANSP);
            lv_draw_vector_dsc_set_fill_color(ctx, lv_color_make(255, 0, 0));
            lv_draw_vector_dsc_set_fill_opa(ctx, LV_OPA_COVER);
            lv_draw_vector_dsc_add_path(ctx, path);
            lv_draw_vector(ctx);

            lv_vector_path_clear(path);

            lv_vector_path_move_to(path, &pts[0]);
            lv_vector_path_cubic_to(path, &pts[1], &pts[2], &pts[3]);

            lv_draw_vector_dsc_set_stroke_color(ctx, m_pen.color);
            lv_draw_vector_dsc_set_stroke_opa(ctx, LV_OPA_COVER);
            lv_draw_vector_dsc_set_stroke_width(ctx, m_pen.width);
            lv_draw_vector_dsc_set_fill_opa(ctx, LV_OPA_TRANSP);

            lv_draw_vector_dsc_add_path(ctx, path);

            lv_draw_vector_dsc_set_stroke_opa(ctx, LV_OPA_TRANSP);
            lv_draw_vector_dsc_set_fill_opa(ctx, LV_OPA_COVER);

            lv_draw_vector(ctx);
            lv_vector_path_delete(path);
            lv_draw_vector_dsc_delete(ctx);
        }

        void draw_image(void* src) {
            lv_draw_image_dsc_t dsc;
            lv_draw_image_dsc_init(&dsc);
            dsc.src = src;

            lv_area_t area;
            lv_obj_get_coords(m_obj, &area);
            lv_draw_image(m_layer, &dsc, &area);
        }

        void draw_text(std::string text) {
            lv_draw_label_dsc_t dsc;
            lv_draw_label_dsc_init(&dsc);

            dsc.color = m_pen.color;
            dsc.text  = text.c_str();

            lv_area_t area;
            lv_obj_get_coords(m_obj, &area);
            lv_draw_label(m_layer, &dsc, &area);
        }

        void draw_fill_rect(geom::rect_t<> rect) {
            lv_draw_fill_dsc_t dsc;
            lv_draw_fill_dsc_init(&dsc);
            dsc.color = m_brush.color;

            auto x    = rect.top_left.x;
            auto y    = rect.top_left.y;
            auto w    = rect.width;
            auto h    = rect.height;

            x += m_offset_x;
            y += m_offset_y;

            lv_area_t coords;
            coords.x1 = x;
            coords.y1 = y;
            coords.x2 = x + w;
            coords.y2 = y + h;

            lv_draw_fill(m_layer, &dsc, &coords);
        }
    private:
        lv_event_t* m_event;
        lv_obj_t* m_obj;
        lv_layer_t* m_layer = nullptr;

        int m_offset_x;
        int m_offset_y;

        GraphicsPen m_pen;
        GraphicsBrush m_brush;
    };

}  // namespace graphics::v2
