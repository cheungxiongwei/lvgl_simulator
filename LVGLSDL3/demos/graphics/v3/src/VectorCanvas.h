#pragma once

#include "lvgl.h"
#include <vector>
#include <stack>

class VectorCanvas
{
public:
    VectorCanvas(int width, int height, lv_obj_t* parent) {
        lv_draw_buf_t* draw_buf = lv_draw_buf_create(width, height, LV_COLOR_FORMAT_NATIVE_WITH_ALPHA, LV_STRIDE_AUTO);
        lv_draw_buf_clear(draw_buf, NULL);

        m_canvas = lv_canvas_create(parent);
        lv_canvas_set_draw_buf(m_canvas, draw_buf);
        lv_obj_add_event_cb(
            m_canvas,
            [](auto e)
            {
                lv_obj_t* obj           = lv_event_get_target_obj(e);
                lv_draw_buf_t* draw_buf = lv_canvas_get_draw_buf(obj);
                lv_draw_buf_destroy(draw_buf);
            },
            LV_EVENT_DELETE,
            NULL);
        update();
    }

    VectorCanvas(lv_obj_t* parent) {
        m_canvas = lv_obj_create(parent);
        lv_obj_center(m_canvas);
        lv_obj_set_style_border_width(m_canvas, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_all(m_canvas, 12, LV_PART_MAIN);
        lv_obj_set_style_margin_all(m_canvas, 12, LV_PART_MAIN);
        lv_obj_set_size(m_canvas, LV_PCT(100), LV_PCT(100));
        lv_obj_update_layout(m_canvas);

        lv_obj_add_event_cb(
            m_canvas,
            [](auto e)
            {
                auto self         = static_cast<VectorCanvas*>(lv_event_get_user_data(e));
                lv_layer_t* layer = lv_event_get_layer(e);

                VectorPainter painter(self, layer);
                self->paint(&painter);
            },
            LV_EVENT_DRAW_MAIN,
            this);
    }

    ~VectorCanvas() { lv_obj_delete(m_canvas); }

    lv_obj_t* canvas() { return m_canvas; }

    void update() {
        lv_layer_t layer;
        lv_canvas_init_layer(m_canvas, &layer);

        VectorPainter painter(this, &layer);
        paint(&painter);

        lv_canvas_finish_layer(m_canvas, &layer);
    }

    struct Pen {
        float width      = 1;
        lv_color_t color = lv_color_make(0x00, 0xff, 0x00);

        void set_pen_width(float width) { width = width; }

        void set_pen_color(lv_color_t color) { color = color; }
    };

    struct Brush {
        lv_color_t color = lv_color_lighten(lv_color_black(), 50);

        void set_brush_color(lv_color_t color) { color = color; }
    };

    class VectorPainter
    {
    public:
        VectorPainter(VectorCanvas* canvas, lv_layer_t* layer) : m_canvas(canvas) {
            m_path = lv_vector_path_create(LV_VECTOR_PATH_QUALITY_MEDIUM);
            m_ctx  = lv_draw_vector_dsc_create(layer);
        }

        ~VectorPainter() {
            lv_draw_vector(m_ctx);
            lv_vector_path_delete(m_path);
            lv_draw_vector_dsc_delete(m_ctx);
        }

        void set_pen_width(float width) { m_pen.width = width; }

        void set_pen_color(lv_color_t color) { m_pen.color = color; }

        void set_brush(lv_color_t color) { m_brush.color = color; }

        void _set_pen_width(float width) { lv_draw_vector_dsc_set_stroke_width(m_ctx, m_pen.width); }

        void _set_pen_color(lv_color_t color) {
            lv_draw_vector_dsc_set_stroke_color(m_ctx, m_pen.color);
            lv_draw_vector_dsc_set_stroke_opa(m_ctx, LV_OPA_COVER);
        }

        void _set_brush() {
            lv_draw_vector_dsc_set_fill_color(m_ctx, m_brush.color);
            lv_draw_vector_dsc_set_fill_opa(m_ctx, LV_OPA_COVER);
        }

        void set_no_brush() { lv_draw_vector_dsc_set_fill_opa(m_ctx, LV_OPA_TRANSP); }

        void fill_rect(lv_area_t const& rect) {
            lv_draw_vector_dsc_set_fill_color(m_ctx, m_brush.color);
            lv_draw_vector_dsc_clear_area(m_ctx, &rect);
        };

        void draw_point(lv_fpoint_t const& point) {
            reset_path();

            lv_vector_path_append_circle(m_path, &point, m_pen.width, m_pen.width);

            lv_draw_vector_dsc_set_fill_color(m_ctx, m_pen.color);
            lv_draw_vector_dsc_add_path(m_ctx, m_path);
        }

        void draw_line(lv_fpoint_t const& p1, lv_fpoint_t const& p2) {
            reset_path();

            lv_vector_path_move_to(m_path, &p1);
            lv_vector_path_line_to(m_path, &p1);
            lv_vector_path_line_to(m_path, &p2);

            lv_draw_vector_dsc_set_stroke_width(m_ctx, m_pen.width);
            lv_draw_vector_dsc_set_stroke_color(m_ctx, m_pen.color);
            lv_draw_vector_dsc_set_stroke_opa(m_ctx, LV_OPA_COVER);
            lv_draw_vector_dsc_set_fill_opa(m_ctx, LV_OPA_TRANSP);

            lv_draw_vector_dsc_add_path(m_ctx, m_path);
        }

        void draw_polyline(std::vector<lv_fpoint_t> const& lines) {
            if (lines.size() < 2)
                return;

            reset_path();

            lv_vector_path_move_to(m_path, &lines[0]);
            for (size_t i = 1; i < lines.size(); ++i) {
                lv_vector_path_line_to(m_path, &lines[i]);
            }

            lv_draw_vector_dsc_set_stroke_width(m_ctx, m_pen.width);
            lv_draw_vector_dsc_set_stroke_color(m_ctx, m_pen.color);
            lv_draw_vector_dsc_set_stroke_opa(m_ctx, LV_OPA_COVER);
            lv_draw_vector_dsc_set_fill_opa(m_ctx, LV_OPA_TRANSP);

            lv_draw_vector_dsc_add_path(m_ctx, m_path);
        }

        void reset_path() { lv_vector_path_clear(m_path); }

        void translate(float tx, float ty) { lv_draw_vector_dsc_translate(m_ctx, tx, ty); }

        void scale(float sx, float sy) { lv_draw_vector_dsc_scale(m_ctx, sx, sy); }

        enum class CoordinateSystem {
            LeftTop,  // 默认
            RightTop,
            LeftBottom,
            RightBottom,
            Center
        };

        void set_coordinate_system(CoordinateSystem cs, float width, float height) {
            // 重置矩阵（很关键）
            lv_draw_vector_dsc_identity(m_ctx);

            switch (cs) {
                case CoordinateSystem::LeftTop:
                    // 默认，无需处理
                    break;

                case CoordinateSystem::RightTop:
                    // 原点移到右上 + x 反转
                    lv_draw_vector_dsc_translate(m_ctx, width, 0);
                    lv_draw_vector_dsc_scale(m_ctx, -1, 1);
                    break;

                case CoordinateSystem::LeftBottom:
                    // 原点移到左下 + y 反转
                    lv_draw_vector_dsc_translate(m_ctx, 0, height);
                    lv_draw_vector_dsc_scale(m_ctx, 1, -1);
                    break;

                case CoordinateSystem::RightBottom:
                    // 原点移到右下 + xy 反转
                    lv_draw_vector_dsc_translate(m_ctx, width, height);
                    lv_draw_vector_dsc_scale(m_ctx, -1, -1);
                    break;

                case CoordinateSystem::Center:
                    // 原点移到中心 + y 向上（更符合数学坐标）
                    lv_draw_vector_dsc_translate(m_ctx, width / 2.0f, height / 2.0f);
                    lv_draw_vector_dsc_scale(m_ctx, 1, -1);
                    break;
            }
        }

        void set_canvas_matrix(lv_matrix_t m) { m_canvas_matrix = m; }

        void save() { m_state.push({m_pen, m_brush}); }

        void restore() {
            if (!m_state.empty()) {
                auto& state = m_state.top();
                m_pen       = state.pen;
                m_brush     = state.brush;
                m_state.pop();
            }
        }
    private:
        Pen m_pen;
        Brush m_brush;
        VectorCanvas* m_canvas;
        lv_vector_path_t* m_path;
        lv_draw_vector_dsc_t* m_ctx;

        lv_matrix_t m_canvas_matrix;

        struct State {
            Pen pen;
            Brush brush;
        };

        std::stack<State> m_state;
    };

    virtual void paint(VectorPainter* painter) {
        lv_area_t rect;
        lv_obj_get_content_coords(m_canvas, &rect);
        painter->fill_rect(rect);

        float w = rect.x2 - rect.x1;
        float h = rect.y2 - rect.y1;

        painter->translate(rect.x1, rect.y1);  // 先对齐到 content 区域
        painter->set_coordinate_system(VectorPainter::CoordinateSystem::LeftBottom, w, h);

        float cx = (w) / 2.0f;
        float cy = (h) / 2.0f;

        painter->set_pen_width(1);

        painter->save();
        painter->set_pen_width(3);
        painter->draw_point({cx, cy});
        painter->draw_point({0, 0});
        painter->draw_point({24, 24});
        painter->restore();

        painter->draw_line({30, 30}, {100, 100});
        painter->draw_polyline(std::vector<lv_fpoint_t>{
            {100,  10},
            {222, 121},
            {333,  11},
            {388, 138},
            {411,  15},
            {611, 256},
        });
    }

    // mapFromGlobal
private:
    lv_obj_t* m_canvas = nullptr;
};
