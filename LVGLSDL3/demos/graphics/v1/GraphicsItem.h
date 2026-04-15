#pragma once

#include "pch.h"
#include "Transform.h"

namespace graphics::v1
{

    class GraphicsFactory
    {
    public:
        static lv_obj_t* create_object(lv_point_t pos, int width, int height, lv_obj_t* parent) {
            lv_obj_t* obj = lv_obj_create(parent);
            lv_obj_set_scroll_dir(obj, LV_DIR_NONE);
            lv_obj_set_flag(obj, LV_OBJ_FLAG_EVENT_BUBBLE, true);
            lv_obj_set_flag(obj, LV_OBJ_FLAG_CLICKABLE, true);
            lv_obj_set_pos(obj, pos.x, pos.y);
            lv_obj_set_size(obj, width, height);
            lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN);
            lv_obj_set_style_margin_all(obj, 0, LV_PART_MAIN);
            lv_obj_set_style_border_color(obj, lv_palette_main(LV_PALETTE_ORANGE), LV_PART_MAIN);
            lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN);
            return obj;
        }

        static lv_obj_t* create_text(lv_point_t pos, char const* text, lv_obj_t* parent) {
            lv_obj_t* obj = lv_label_create(parent);
            lv_obj_set_flag(obj, LV_OBJ_FLAG_EVENT_BUBBLE, true);
            lv_obj_set_flag(obj, LV_OBJ_FLAG_CLICKABLE, true);
            lv_obj_set_width(obj, LV_SIZE_CONTENT);
            lv_label_set_text(obj, text);
            lv_obj_set_pos(obj, pos.x, pos.y);
            return obj;
        }

        static lv_obj_t* create_line(lv_point_t pos, lv_point_precise_t p1, lv_point_precise_t p2, lv_obj_t* parent) {
            lv_point_precise_t list[2] = {p1, p2};
            return create_polygon(pos, list, false, parent);
        }

        static lv_obj_t* create_rect(lv_point_t pos, int width, int height, lv_obj_t* parent) {
            lv_obj_t* obj = lv_obj_create(parent);
            lv_obj_set_scroll_dir(obj, LV_DIR_NONE);
            lv_obj_set_flag(obj, LV_OBJ_FLAG_EVENT_BUBBLE, true);
            lv_obj_set_pos(obj, pos.x, pos.y);
            lv_obj_set_size(obj, width, height);
            lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN);
            lv_obj_set_style_margin_all(obj, 0, LV_PART_MAIN);
            lv_obj_set_style_border_color(obj, lv_palette_main(LV_PALETTE_GREY), LV_PART_MAIN);
            lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN);
            return obj;
        }

        template <int size>
        static lv_obj_t* create_polygon(lv_point_t pos,
                                        lv_point_precise_t (&point)[size],
                                        bool close,
                                        lv_obj_t* parent) {
            lv_obj_t* obj = lv_line_create(parent);
            lv_obj_set_scroll_dir(obj, LV_DIR_NONE);
            lv_obj_set_flag(obj, LV_OBJ_FLAG_EVENT_BUBBLE, true);
            lv_obj_set_flag(obj, LV_OBJ_FLAG_CLICKABLE, true);
            lv_obj_set_pos(obj, pos.x, pos.y);

            lv_point_precise_t* points = (lv_point_precise_t*)lv_malloc_zeroed(sizeof(lv_point_precise_t) * (size + 1));
            std::copy(point, point + size, points);
            if (close) {
                points[size] = points[0];
            }
            lv_line_set_points(obj, points, close ? size + 1 : size);
            lv_obj_add_event_cb(
                obj,
                [](auto e)
                {
                    auto* p = lv_event_get_user_data(e);
                    if (p) {
                        lv_free(p);
                        p = nullptr;
                    }
                },
                LV_EVENT_DELETE,
                points);
            return obj;
        }
    };

    class GraphicsItem
    {
    public:
        GraphicsItem() = default;
        virtual ~GraphicsItem() {};

        void rotate(float deg) {
            m_transform.deg += deg;
            while (m_transform.deg >= 360)
                m_transform.deg -= 360;
            while (m_transform.deg < 0)
                m_transform.deg += 360;
            m_transform.apply(m_obj);
        }

        void set_rotation(float deg) {
            m_transform.deg = deg;
            while (m_transform.deg >= 360)
                m_transform.deg -= 360;
            while (m_transform.deg < 0)
                m_transform.deg += 360;
            m_transform.apply(m_obj);
        }

        lv_obj_t* to_object() { return m_obj; }

        Transform& transform() { return m_transform; }

        lv_obj_t* m_obj = nullptr;
        Transform m_transform;
    };

    class GraphicsLineItem : public GraphicsItem
    {
    public:
        GraphicsLineItem(int x, int y, lv_point_precise_t p1, lv_point_precise_t p2, lv_obj_t* parent) {
            m_obj = GraphicsFactory::create_line({x, y}, p1, p2, parent);
        }
    };

    class GraphicsRectItem : public GraphicsItem
    {
    public:
        GraphicsRectItem(int x, int y, int width, int height, lv_obj_t* parent) {
            m_obj = GraphicsFactory::create_rect({x, y}, width, height, parent);
        }
    };

    class GraphicsPolygonItem : public GraphicsItem
    {
    public:
        template <int size>
        GraphicsPolygonItem(int x, int y, lv_point_precise_t (&list)[size], bool is_close, lv_obj_t* parent) {
            m_obj = GraphicsFactory::create_polygon<size>({x, y}, list, is_close, parent);
        }
    };

    class GraphicsTextItem : public GraphicsItem
    {
    public:
        GraphicsTextItem(int x, int y, char const* text, lv_obj_t* parent) {
            m_obj = GraphicsFactory::create_text({x, y}, text, parent);
        }
    };
}  // namespace graphics::v1
