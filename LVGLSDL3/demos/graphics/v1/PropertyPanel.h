#pragma once

#include "pch.h"
#include "Transform.h"
#include "GraphicsItem.h"

namespace graphics::v1
{
    class PropertyPanel
    {
    public:
        struct PropertyField {
            char const* name;
            lv_obj_t* layout;
            lv_obj_t* label;
            lv_obj_t* input;

            void set_value(int v) {
                char buf[32];
                sprintf(buf, "%d", v);
                lv_textarea_set_text(input, buf);
            }

            void set_value(float v) {
                char buf[32];
                sprintf(buf, "%.1f", v);
                lv_textarea_set_text(input, buf);
            }
        };

        PropertyPanel(lv_obj_t* parent) : m_ref_item(nullptr) {
            m_layout = lv_obj_create(parent);
            lv_obj_set_size(m_layout, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_flex_flow(m_layout, LV_FLEX_FLOW_COLUMN);
            lv_obj_set_style_pad_gap(m_layout, 6, 0);
            lv_obj_set_flex_align(m_layout, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_SPACE_EVENLY);
            lv_obj_set_style_bg_color(m_layout, lv_color_make(232, 230, 230), 0);
            lv_obj_set_style_opa(m_layout, LV_OPA_COVER, 0);

            lv_obj_t* title = lv_label_create(m_layout);
            lv_label_set_text(title, "Property");

            create_property(m_layout, "x").set_value(0);
            create_property(m_layout, "y").set_value(0);
            create_property(m_layout, "width").set_value(0);
            create_property(m_layout, "height").set_value(0);
            create_property(m_layout, "tx").set_value(0.0f);
            create_property(m_layout, "ty").set_value(0.0f);
            create_property(m_layout, "sx").set_value(1.0f);
            create_property(m_layout, "sy").set_value(1.0f);
            create_property(m_layout, "deg").set_value(0.0f);

            set_visible(false);
        }

        ~PropertyPanel() { lv_obj_delete(m_layout); }

        lv_obj_t* to_object() { return m_layout; }

        PropertyField create_property(lv_obj_t* parent, char const* name) {
            lv_obj_t* layout = lv_obj_create(parent);
            lv_obj_remove_style_all(layout);
            lv_obj_set_size(layout, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_flex_flow(layout, LV_FLEX_FLOW_ROW);
            lv_obj_set_style_pad_gap(layout, 6, 0);
            lv_obj_set_flex_align(layout, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_SPACE_EVENLY);

            lv_obj_t* label = lv_label_create(layout);
            lv_label_set_text(label, name);
            lv_obj_set_width(label, 60);

            lv_obj_t* input = lv_textarea_create(layout);
            lv_obj_set_scroll_dir(input, LV_DIR_NONE);
            lv_textarea_set_one_line(input, true);
            lv_textarea_set_accepted_chars(input, "0123456789.-");
            lv_obj_set_style_min_width(input, 60, 0);
            lv_obj_set_style_max_width(input, 60, 0);
            lv_obj_set_style_pad_all(input, 0, 0);
            lv_obj_set_style_pad_top(input, 4, 0);
            lv_obj_set_style_pad_bottom(input, 4, 0);
            lv_obj_set_style_pad_left(input, 8, 0);
            lv_obj_set_style_pad_right(input, 8, 0);
            lv_obj_set_style_margin_all(input, 0, 0);
            lv_obj_set_style_margin_top(input, 2, 0);
            lv_obj_set_style_margin_bottom(input, 2, 0);

            PropertyField field;
            field.name   = name;
            field.layout = layout;
            field.label  = label;
            field.input  = input;

            lv_obj_add_event_cb(field.input, PropertyPanel::event, LV_EVENT_READY, this);
            m_properties[name] = field;
            return field;
        }

        void show_for_item(GraphicsItem* item) {
            if (item == nullptr) {
                set_visible(false);
                return;
            }
            set_visible(true);
            m_ref_item = item;
            reset_properties();
        }

        void reset_properties() {
            if (!m_ref_item)
                return;
            lv_obj_t* object = m_ref_item->to_object();
            m_properties["x"].set_value(lv_obj_get_x(object));
            m_properties["y"].set_value(lv_obj_get_y(object));
            m_properties["width"].set_value(lv_obj_get_width(object));
            m_properties["height"].set_value(lv_obj_get_height(object));
            update_transform_ui();
        }

        void update_transform_ui() {
            if (!m_ref_item)
                return;
            Transform& transform = m_ref_item->transform();
            m_properties["tx"].set_value(transform.tx);
            m_properties["ty"].set_value(transform.ty);
            m_properties["sx"].set_value(transform.sx);
            m_properties["sy"].set_value(transform.sy);
            m_properties["deg"].set_value(transform.deg);
        }

        void apply_transform() {
            if (!m_ref_item)
                return;
            Transform& transform = m_ref_item->transform();
            transform.tx         = strtof(lv_textarea_get_text(m_properties["tx"].input), nullptr);
            transform.ty         = strtof(lv_textarea_get_text(m_properties["ty"].input), nullptr);
            transform.sx         = strtof(lv_textarea_get_text(m_properties["sx"].input), nullptr);
            transform.sy         = strtof(lv_textarea_get_text(m_properties["sy"].input), nullptr);
            transform.deg        = strtof(lv_textarea_get_text(m_properties["deg"].input), nullptr);
            transform.apply(m_ref_item->to_object());
            update_transform_ui();
        }

        void set_visible(bool visible) { lv_obj_set_flag(m_layout, LV_OBJ_FLAG_HIDDEN, !visible); }

        static void event(lv_event_t* e) {
            PropertyPanel* self = (PropertyPanel*)lv_event_get_user_data(e);
            if (!self || !self->m_ref_item)
                return;

            lv_obj_t* ta     = lv_event_get_current_target_obj(e);
            std::string text = lv_textarea_get_text(ta);
            lv_obj_t* object = self->m_ref_item->to_object();

            for (auto& [key, field] : self->m_properties) {
                if (field.input != ta)
                    continue;
                if (key.compare("x") == 0)
                    lv_obj_set_x(object, std::stoi(text));
                else if (key.compare("y") == 0)
                    lv_obj_set_y(object, std::stoi(text));
                else if (key.compare("width") == 0)
                    lv_obj_set_width(object, std::stoi(text));
                else if (key.compare("height") == 0)
                    lv_obj_set_height(object, std::stoi(text));
                else
                    self->apply_transform();
                break;
            }
        };

        GraphicsItem* m_ref_item = nullptr;
        lv_obj_t* m_layout       = nullptr;
        std::map<std::string, PropertyField> m_properties;
    };
}  // namespace graphics::v1
