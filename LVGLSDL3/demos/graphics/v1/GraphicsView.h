#pragma once

#include "pch.h"
#include "GraphicsScene.h"

namespace graphics::v1
{
    class GraphicsView
    {
    public:
        GraphicsView(lv_obj_t* parent) {
            m_obj = lv_obj_create(parent);
            lv_obj_set_scroll_dir(m_obj, LV_DIR_NONE);
            lv_obj_set_size(m_obj, LV_PCT(100), LV_PCT(100));
            lv_obj_set_style_radius(m_obj, 0, 0);
            lv_obj_set_style_pad_all(m_obj, 0, 0);
            lv_obj_set_style_margin_all(m_obj, 0, 0);
            lv_obj_set_style_border_color(m_obj, lv_color_make(255, 0, 0), 0);
            lv_obj_set_style_border_width(m_obj, 1, 0);

            lv_obj_add_event_cb(
                m_obj,
                [](lv_event_t* e)
                {
                    lv_event_code_t code = lv_event_get_code(e);
                    if (!(code == LV_EVENT_PRESSED || code == LV_EVENT_RELEASED || code == LV_EVENT_PRESSING))
                        return;

                    GraphicsView* view = (GraphicsView*)lv_event_get_user_data(e);
                    lv_indev_t* indev  = lv_indev_active();
                    if (indev == nullptr)
                        return;

                    lv_point_t point;
                    lv_indev_get_point(indev, &point);

                    if (code == LV_EVENT_PRESSED)
                        view->mouse_press(e, point);
                    else if (code == LV_EVENT_RELEASED)
                        view->mouse_release(e, point);
                    else if (code == LV_EVENT_PRESSING)
                        view->mouse_move(e, point);
                },
                LV_EVENT_ALL,
                this);
        }

        void mouse_press(lv_event_t* e, lv_point_t point) {
            if (m_scene)
                m_scene->mouse_press(e, point);
        }

        void mouse_move(lv_event_t* e, lv_point_t point) {
            if (m_scene)
                m_scene->mouse_move(e, point);
        }

        void mouse_release(lv_event_t* e, lv_point_t point) {
            if (m_scene)
                m_scene->mouse_release(e, point);
        }

        void set_scene(GraphicsScene* scene) { m_scene = scene; }

        lv_obj_t* to_object() { return m_obj; }
    private:
        lv_obj_t* m_obj = nullptr;
        Transform m_transform;
        GraphicsScene* m_scene = nullptr;
    };

}  // namespace graphics::v1
