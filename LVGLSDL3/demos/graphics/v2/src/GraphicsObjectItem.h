#pragma once

#include "GraphicsItem.h"
#include "GraphicsPainter.h"
#include "GraphicsGeom.h"
#include "lvgl/src/misc/lv_text_private.h"

namespace graphics::v2
{
    class GraphicsObjectItem : public graphics::v1::GraphicsItem
    {
    public:
        GraphicsObjectItem(int x, int y, int width, int height, lv_obj_t* parent) {
            m_obj = graphics::v1::GraphicsFactory::create_object({x, y}, width, height, parent);
            lv_obj_set_style_radius(m_obj, 0, 0);
            lv_obj_set_style_border_width(m_obj, 0, 0);

            lv_obj_set_style_bg_opa(m_obj, LV_OPA_TRANSP, 0);

            lv_obj_add_event_cb(
                m_obj,
                [](auto e)
                {
                    if (!lv_event_get_layer(e)) {
                        printf("layer is null\n");
                        return;
                    }

                    GraphicsPainter painter(e);
                    GraphicsObjectItem* object = (GraphicsObjectItem*)lv_event_get_user_data(e);
                    object->paint(&painter);
                },
                LV_EVENT_DRAW_MAIN,
                this);

            lv_obj_add_event_cb(
                m_obj,
                [](auto e)
                {
                    int32_t* ext = (int32_t*)lv_event_get_param(e);
                    *ext += 2;
                },
                LV_EVENT_REFR_EXT_DRAW_SIZE,
                this);
        }

        virtual void paint(GraphicsPainter* painter) = 0;

        lv_area_t make_area(geom::point_t<> point) {
            int32_t x_min = (int)point.x;
            int32_t y_min = (int)point.y;
            int32_t x_max = (int)point.x;
            int32_t y_max = (int)point.y;

            if (x_max < x_min)
                std::swap(x_min, x_max);
            if (y_max < y_min)
                std::swap(y_min, y_max);

            return {x_min, y_min, x_max, y_max};
        }

        lv_area_t make_area(geom::line_t<> line) {
            int32_t x_min = (int)line.start.x;
            int32_t y_min = (int)line.start.y;
            int32_t x_max = (int)line.end.x;
            int32_t y_max = (int)line.end.y;

            if (x_max < x_min)
                std::swap(x_min, x_max);
            if (y_max < y_min)
                std::swap(y_min, y_max);

            return {x_min, y_min, x_max, y_max};
        }

        lv_area_t make_area(geom::polyline_t<> polyline) {
            if (polyline.points.empty()) {
                return {0, 0, 1, 1};
            }

            int32_t x_min = static_cast<int32_t>(polyline.points[0].x);
            int32_t y_min = static_cast<int32_t>(polyline.points[0].y);
            int32_t x_max = x_min;
            int32_t y_max = y_min;

            for (auto const& p : polyline.points) {
                int32_t x = static_cast<int32_t>(p.x);
                int32_t y = static_cast<int32_t>(p.y);

                if (x < x_min)
                    x_min = x;
                if (x > x_max)
                    x_max = x;
                if (y < y_min)
                    y_min = y;
                if (y > y_max)
                    y_max = y;
            }

            return {x_min, y_min, x_max, y_max};
        }

        lv_area_t make_area(geom::triangle_t<> tri) {
            int32_t x_min = (int32_t)tri.a.x;
            int32_t y_min = (int32_t)tri.a.y;
            int32_t x_max = x_min;
            int32_t y_max = y_min;

            auto update   = [&](auto const& p)
            {
                int32_t x = (int32_t)p.x;
                int32_t y = (int32_t)p.y;

                x_min     = std::min(x_min, x);
                x_max     = std::max(x_max, x);
                y_min     = std::min(y_min, y);
                y_max     = std::max(y_max, y);
            };

            update(tri.b);
            update(tri.c);

            return {x_min, y_min, x_max, y_max};
        }

        lv_area_t make_area(geom::arc_t<> arc) {
            return {0, 0, static_cast<int>(arc.radius * 2.0f), static_cast<int>(arc.radius * 2.0f)};
        }

        lv_area_t make_area(geom::polygon_t<> poly) {
            if (poly.vertices.empty())
                return {0, 0, 0, 0};

            int32_t x_min = (int32_t)poly.vertices[0].x;
            int32_t y_min = (int32_t)poly.vertices[0].y;
            int32_t x_max = x_min;
            int32_t y_max = y_min;

            for (auto const& p : poly.vertices) {
                int32_t x = (int32_t)p.x;
                int32_t y = (int32_t)p.y;

                x_min     = std::min(x_min, x);
                x_max     = std::max(x_max, x);
                y_min     = std::min(y_min, y);
                y_max     = std::max(y_max, y);
            }

            return {x_min, y_min, x_max, y_max};
        }

        lv_area_t make_area(geom::rect_t<> rect) {
            int32_t x1    = (int32_t)rect.top_left.x;
            int32_t y1    = (int32_t)rect.top_left.y;
            int32_t x2    = (int32_t)(rect.top_left.x + rect.width);
            int32_t y2    = (int32_t)(rect.top_left.y + rect.height);

            int32_t x_min = std::min(x1, x2);
            int32_t x_max = std::max(x1, x2);
            int32_t y_min = std::min(y1, y2);
            int32_t y_max = std::max(y1, y2);

            return {x_min, y_min, x_max, y_max};
        }

        lv_area_t make_area(geom::circle_t<> circle) {
            return {0, 0, static_cast<int>(circle.radius * 2.0f), static_cast<int>(circle.radius * 2.0f)};
        }
    };

    class GraphicsPointWidget : public GraphicsObjectItem
    {
    public:
        GraphicsPointWidget(int x, int y, geom::point_t<> point, lv_obj_t* parent)
            : GraphicsObjectItem(x, y, 1, 1, parent) {
            m_line = {point, point};
            m_line.end.x += 1;
            m_line.end.y += 1;

            lv_area_t area = make_area(m_line);
            lv_obj_set_size(m_obj, area.x1 + (area.x2 - area.x1), area.y1 + (area.y2 - area.y1));
        }

        void paint(GraphicsPainter* painter) {
            painter->set_brush_color(lv_color_make(0, 0xff, 0xff));
            painter->set_pen_color(lv_color_make(0, 0xff, 0));
            painter->draw_line(m_line);
        }

        geom::line_t<float> m_line = {
            {0, 0},
            {1, 1}
        };
    };

    class GraphicsLineWidget : public GraphicsObjectItem
    {
    public:
        GraphicsLineWidget(int x, int y, geom::line_t<> line, lv_obj_t* parent)
            : GraphicsObjectItem(x, y, 1, 1, parent) {
            lv_area_t area = make_area(line);
            lv_obj_set_size(m_obj, area.x1 + (area.x2 - area.x1), area.y1 + (area.y2 - area.y1));
            m_line = line;
        }

        void paint(GraphicsPainter* painter) {
            painter->set_brush_color(lv_color_make(0, 0xff, 0xff));
            painter->set_pen_color(lv_color_make(0, 0xff, 0));
            painter->draw_line(m_line);
        }

        geom::line_t<> m_line;
    };

    class GraphicsPolylineWidget : public GraphicsObjectItem
    {
    public:
        GraphicsPolylineWidget(int x, int y, geom::polyline_t<> polyline, lv_obj_t* parent)
            : GraphicsObjectItem(x, y, 1, 1, parent) {
            lv_area_t area = make_area(polyline);
            lv_obj_set_size(m_obj, area.x1 + (area.x2 - area.x1), area.y1 + (area.y2 - area.y1));
            m_polyline = polyline;
        }

        void paint(GraphicsPainter* painter) {
            painter->set_brush_color(lv_color_make(0, 0xff, 0xff));
            painter->set_pen_color(lv_color_make(0, 0xff, 0));
            painter->draw_polyline(m_polyline);
        }

        geom::polyline_t<> m_polyline;
    };

    class GraphicsArcWidget : public GraphicsObjectItem
    {
    public:
        GraphicsArcWidget(int x, int y, geom::arc_t<> arc, lv_obj_t* parent) : GraphicsObjectItem(x, y, 1, 1, parent) {
            lv_area_t area = make_area(arc);
            lv_obj_set_size(m_obj, area.x1 + (area.x2 - area.x1), area.y1 + (area.y2 - area.y1));
            m_arc = arc;
        }

        void paint(GraphicsPainter* painter) {
            painter->set_brush_color(lv_color_make(0, 0xff, 0xff));
            painter->set_pen_color(lv_color_make(0, 0xff, 0));
            painter->draw_arc(m_arc);
        }

        geom::arc_t<> m_arc;
    };

    class GraphicsTriangleWidget : public GraphicsObjectItem
    {
    public:
        GraphicsTriangleWidget(int x, int y, geom::triangle_t<> triangle, lv_obj_t* parent)
            : GraphicsObjectItem(x, y, 1, 1, parent) {
            lv_area_t area = make_area(triangle);
            lv_obj_set_size(m_obj, area.x1 + (area.x2 - area.x1), area.y1 + (area.y2 - area.y1));
            m_triangle = triangle;
        }

        void paint(GraphicsPainter* painter) {
            painter->set_brush_color(lv_color_make(0, 0xff, 0xff));
            painter->set_pen_color(lv_color_make(0, 0xff, 0));
            painter->draw_triangle(m_triangle);
        }

        geom::triangle_t<> m_triangle;
    };

    class GraphicsCircleWidget : public GraphicsObjectItem
    {
    public:
        GraphicsCircleWidget(int x, int y, geom::circle_t<> circle, lv_obj_t* parent)
            : GraphicsObjectItem(x, y, 1, 1, parent) {
            lv_area_t area = make_area(circle);
            lv_obj_set_size(m_obj, area.x2, area.y2);
            m_circle = circle;
        }

        void paint(GraphicsPainter* painter) {
            painter->set_brush_color(lv_color_make(0, 0xff, 0xff));
            painter->set_pen_color(lv_color_make(0, 0xff, 0));
            painter->draw_circle(m_circle);
        }

        geom::circle_t<> m_circle;
    };

    class GraphicsRectWidget : public GraphicsObjectItem
    {
    public:
        GraphicsRectWidget(int x, int y, geom::rect_t<> rect, lv_obj_t* parent)
            : GraphicsObjectItem(x, y, 1, 1, parent) {
            lv_area_t area = make_area(rect);
            lv_obj_set_size(m_obj, area.x1 + (area.x2 - area.x1), area.y1 + (area.y2 - area.y1));
            m_rect = rect;
        }

        void paint(GraphicsPainter* painter) {
            painter->set_brush_color(lv_color_make(0, 0xff, 0xff));
            painter->set_pen_color(lv_color_make(0, 0xff, 0));
            painter->draw_rect(m_rect);
        }

        geom::rect_t<> m_rect;
    };

    class GraphicsPolygonWidget : public GraphicsObjectItem
    {
    public:
        GraphicsPolygonWidget(int x, int y, geom::polygon_t<> polygon, lv_obj_t* parent)
            : GraphicsObjectItem(x, y, 1, 1, parent) {
            lv_area_t area = make_area(polygon);
            lv_obj_set_size(m_obj, area.x1 + (area.x2 - area.x1), area.y1 + (area.y2 - area.y1));
            m_polygon = polygon;
        }

        void paint(GraphicsPainter* painter) {
            painter->set_brush_color(lv_color_make(0, 0xff, 0xff));
            painter->set_pen_width(1);
            painter->set_pen_color(lv_color_make(0, 0xff, 0));
            painter->draw_polygon(m_polygon);
        }

        geom::polygon_t<> m_polygon;
    };

    class GraphicsCurveWidget : public GraphicsObjectItem
    {
    public:
        GraphicsCurveWidget(int x, int y, geom::curve_t<> curve, lv_obj_t* parent)
            : GraphicsObjectItem(x, y, 1, 1, parent) {
            m_curve     = curve;

            float max_w = 0;
            float max_h = 0;
            for (int i = 0; i < sizeof(m_pts) / sizeof(lv_fpoint_t); i++) {
                max_w = std::max(max_w, m_pts[i].x);
                max_h = std::max(max_h, m_pts[i].y);
            }

            lv_obj_set_size(m_obj, max_w, max_h);
        }

        void paint(GraphicsPainter* painter) {
            painter->set_brush_color(lv_color_make(0, 0xff, 0xff));
            painter->set_pen_color(lv_color_make(0, 0xff, 0));
            painter->draw_curve(m_curve, m_pts);
        }

        geom::curve_t<> m_curve;

        std::vector<lv_fpoint_t> m_pts = {
            {  0,   0},
            {200, 200},
            {250, 300},
            {350, 150}
        };
    };

    class GraphicsTextWidget : public GraphicsObjectItem
    {
    public:
        GraphicsTextWidget(int x, int y, std::string text, lv_font_t const* font, lv_obj_t* parent)
            : GraphicsObjectItem(x, y, 1, 1, parent) {
            lv_point_t size_res;
            lv_text_get_size(&size_res, text.c_str(), font, 1, 1, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
            lv_obj_set_size(m_obj, size_res.x, size_res.y);
            m_text = text;
        }

        void paint(GraphicsPainter* painter) {
            painter->set_brush_color(lv_color_make(0, 0xff, 0xff));
            painter->set_pen_width(2);
            painter->set_pen_color(lv_color_make(0, 0xff, 0));
            painter->draw_text(m_text);
        }

        std::string m_text;
    };

    class GraphicsImageWidget : public GraphicsObjectItem
    {
    public:
        GraphicsImageWidget(int x, int y, void const* src, lv_obj_t* parent) : GraphicsObjectItem(x, y, 1, 1, parent) {
            m_src_type = lv_image_src_get_type(src);
            switch (m_src_type) {
                case LV_IMAGE_SRC_FILE:
                    std::println("`LV_IMAGE_SRC_FILE` type found");
                    break;
                case LV_IMAGE_SRC_VARIABLE:
                    std::println("`LV_IMAGE_SRC_VARIABLE` type found");
                    break;
                case LV_IMAGE_SRC_SYMBOL:
                    std::println("`LV_IMAGE_SRC_SYMBOL` type found");
                    break;
                default:
                    std::println("unknown type");
            }

            if (m_src_type == LV_IMAGE_SRC_UNKNOWN) {
                if (src)
                    std::println("unknown image type");
                return;
            }

            lv_result_t res = lv_image_decoder_get_info(src, &m_header);
            if (res != LV_RESULT_OK) {
                if (m_src_type == LV_IMAGE_SRC_FILE) {
                    std::println("failed to get image info: {:s}", (char const*)src);
                } else {
                    std::println("failed to get image info: {:p}", src);
                }
                return;
            }

            if (m_src_type == LV_IMAGE_SRC_FILE) {
            } else {
                return;
            }

            lv_obj_set_size(m_obj, m_header.w, m_header.h);

            m_src = src;

            lv_obj_set_style_border_width(m_obj, 1, 0);
            lv_obj_set_style_bg_opa(m_obj, LV_OPA_COVER, LV_PART_MAIN);
            m_is_valid = true;
        }

        void paint(GraphicsPainter* painter) {
            if (!m_is_valid)
                return;

            painter->set_brush_color(lv_color_make(0, 0xff, 0xff));
            painter->set_pen_width(2);
            painter->set_pen_color(lv_color_make(0, 0xff, 0));
            painter->draw_image((void*)m_src);
        }

        bool m_is_valid = false;

        lv_image_dsc_t m_image;
        lv_image_header_t m_header;
        void const* m_src;
        lv_image_src_t m_src_type;
    };

}  // namespace graphics::v2
