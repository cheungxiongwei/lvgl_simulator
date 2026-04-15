#pragma once

#include "pch.h"
#include "PropertyPanel.h"
#include "Gizmos.h"

namespace graphics::v1
{
    class GraphicsScene
    {
    public:
        GraphicsScene(lv_obj_t* parent) {
            m_obj = lv_obj_create(parent);
            lv_obj_set_scroll_dir(m_obj, LV_DIR_NONE);
            lv_obj_set_size(m_obj, LV_PCT(100), LV_PCT(100));
            lv_obj_set_flag(m_obj, LV_OBJ_FLAG_EVENT_BUBBLE, true);
            lv_obj_set_style_radius(m_obj, 0, 0);
            lv_obj_set_style_pad_all(m_obj, 0, 0);
            lv_obj_set_style_margin_all(m_obj, 0, 0);
            lv_obj_set_style_border_color(m_obj, lv_color_make(0, 255, 0), 0);
            lv_obj_set_style_border_width(m_obj, 1, 0);
        }

        void mouse_press(lv_event_t* e, lv_point_t point) {
            lv_obj_t* active = lv_indev_get_active_obj();
            auto [ok, item]  = contains(active);
            if (!m_interact_gizmos->contains(active)) {
                m_interact_gizmos->selected(ok ? item : nullptr);
            }
            m_interact_gizmos->mouse_press(point);
        }

        void mouse_move(lv_event_t* e, lv_point_t point) {
            if (m_interact_gizmos->has_selected()) {
                m_interact_gizmos->mouse_move(point);
                m_property_panel->show_for_item(m_interact_gizmos->current_selected_item());
            }
        }

        void mouse_release(lv_event_t* e, lv_point_t point) {
            if (m_interact_gizmos->has_selected()) {
                m_interact_gizmos->mouse_release(point);
            }
        }

        void add_item(GraphicsItem* item) { m_items.emplace_back(item); }

        lv_obj_t* to_object() { return m_obj; }

        std::pair<bool, GraphicsItem*> contains(lv_obj_t* obj) {
            for (auto item : m_items) {
                if (item->m_obj == obj)
                    return {true, item};
            }
            return {false, nullptr};
        }

        void set_interact_gizmos(Gizmos* item) { m_interact_gizmos = item; }

        Gizmos* get_interact_gizmos() { return m_interact_gizmos; }

        void set_property_panel(PropertyPanel* item) { m_property_panel = item; }

        PropertyPanel* get_property_panel() { return m_property_panel; }

        lv_obj_t* m_obj = nullptr;
        Transform m_transform;
        std::vector<GraphicsItem*> m_items;
        Gizmos* m_interact_gizmos       = nullptr;
        PropertyPanel* m_property_panel = nullptr;
    };
}  // namespace graphics::v1
