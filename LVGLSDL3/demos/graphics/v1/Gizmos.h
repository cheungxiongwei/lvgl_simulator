#pragma once

#include "pch.h"
#include "GraphicsItem.h"
#include <cmath>

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

namespace graphics::v1
{

    enum anchor_mask_t {
        ANCHOR_LEFT         = 1 << 0,
        ANCHOR_RIGHT        = 1 << 1,
        ANCHOR_TOP          = 1 << 2,
        ANCHOR_BOTTOM       = 1 << 3,
        ANCHOR_TOP_LEFT     = ANCHOR_TOP | ANCHOR_LEFT,
        ANCHOR_TOP_RIGHT    = ANCHOR_TOP | ANCHOR_RIGHT,
        ANCHOR_BOTTOM_LEFT  = ANCHOR_BOTTOM | ANCHOR_LEFT,
        ANCHOR_BOTTOM_RIGHT = ANCHOR_BOTTOM | ANCHOR_RIGHT,
        ANCHOR_CENTER       = ANCHOR_TOP | ANCHOR_BOTTOM | ANCHOR_LEFT | ANCHOR_RIGHT,
        ANCHOR_ROTATE       = 1 << 4,
    };

    class Gizmos
    {
    public:
        struct Handle {
            lv_obj_t* obj;
            lv_align_t align;
            int x_offset;
            int y_offset;
        };

        Gizmos(lv_obj_t* parent) {
            m_bbox = lv_obj_create(parent);
            lv_obj_set_scroll_dir(m_bbox, LV_DIR_NONE);
            lv_obj_set_flag(m_bbox, LV_OBJ_FLAG_EVENT_BUBBLE, true);
            lv_obj_set_flag(m_bbox, LV_OBJ_FLAG_OVERFLOW_VISIBLE, true);
            lv_obj_set_size(m_bbox, 1, 1);
            lv_obj_set_style_radius(m_bbox, 0, LV_PART_MAIN);
            lv_obj_set_style_pad_all(m_bbox, 0, LV_PART_MAIN);
            lv_obj_set_style_margin_all(m_bbox, 0, LV_PART_MAIN);
            lv_obj_set_style_bg_opa(m_bbox, LV_OPA_10, LV_PART_MAIN);
            lv_obj_set_style_bg_color(m_bbox, lv_color_make(0xcc, 0x00, 0x00), LV_PART_MAIN);
            lv_obj_set_style_border_color(m_bbox, lv_palette_main(LV_PALETTE_BLUE), LV_PART_MAIN);
            lv_obj_set_style_border_width(m_bbox, 1, LV_PART_MAIN);
            lv_obj_add_event_cb(
                m_bbox,
                [](lv_event_t* e) { ((Gizmos*)lv_event_get_user_data(e))->dispatch_event(e); },
                LV_EVENT_PRESSED,
                this);
            set_visible(false);

            lv_obj_add_event_cb(
                m_bbox,
                [](lv_event_t* e)
                {
                    int32_t* diameter = (int32_t*)lv_event_get_user_data(e);
                    int32_t* ext      = (int32_t*)lv_event_get_param(e);
                    *ext              = 3 * *diameter;
                },
                LV_EVENT_REFR_EXT_DRAW_SIZE,
                (void*)&k_diameter);

            m_handles["tl"]  = create_handle(&m_tl, LV_ALIGN_TOP_LEFT, -k_radius, -k_radius);
            m_handles["tr"]  = create_handle(&m_tr, LV_ALIGN_TOP_RIGHT, k_radius, -k_radius);
            m_handles["bl"]  = create_handle(&m_bl, LV_ALIGN_BOTTOM_LEFT, -k_radius, k_radius);
            m_handles["br"]  = create_handle(&m_br, LV_ALIGN_BOTTOM_RIGHT, k_radius, k_radius);
            m_handles["rot"] = create_handle(&m_rot, LV_ALIGN_TOP_MID, 0, -2 * k_diameter);

            m_handles["tm"]  = create_handle(&m_tm, LV_ALIGN_TOP_MID, 0, -k_radius);
            m_handles["bm"]  = create_handle(&m_bm, LV_ALIGN_BOTTOM_MID, 0, k_radius);
            m_handles["lm"]  = create_handle(&m_lm, LV_ALIGN_LEFT_MID, -k_radius, 0);
            m_handles["rm"]  = create_handle(&m_rm, LV_ALIGN_RIGHT_MID, k_radius, 0);
        }

        Handle create_handle(lv_obj_t** handle, lv_align_t align, int32_t x_offset, int32_t y_offset) {
            *handle = lv_button_create(m_bbox);
            lv_obj_set_flag(*handle, LV_OBJ_FLAG_EVENT_BUBBLE, true);
            lv_obj_set_size(*handle, k_diameter, k_diameter);
            lv_obj_align(*handle, align, x_offset, y_offset);

            lv_obj_add_event_cb(
                *handle,
                [](lv_event_t* e) { ((Gizmos*)lv_event_get_user_data(e))->dispatch_event(e); },
                LV_EVENT_PRESSED,
                this);
            return {*handle, align, x_offset, y_offset};
        }

        void update_handles_align() {
            for (auto const& [key, value] : m_handles) {
                lv_obj_align(value.obj, value.align, value.x_offset, value.y_offset);
            }
        }

        void set_visible(bool visible) { lv_obj_set_flag(m_bbox, LV_OBJ_FLAG_HIDDEN, !visible); }

        void selected(GraphicsItem* item) {
            m_item = item;
            if (m_item == nullptr) {
                set_visible(false);
                return;
            }
            set_visible(true);

            int x = lv_obj_get_x(item->to_object());
            int y = lv_obj_get_y(item->to_object());
            int w = lv_obj_get_width(item->to_object());
            int h = lv_obj_get_height(item->to_object());

            lv_obj_set_pos(m_bbox, x, y);
            lv_obj_set_size(m_bbox, w, h);
            lv_obj_update_layout(m_bbox);

            m_item->m_transform.apply(m_bbox);
            m_current_target = m_bbox;
        }

        bool has_selected() { return m_item != nullptr; }

        GraphicsItem* current_selected_item() { return m_item; }

        void mouse_press(lv_point_t point) {
            if (m_item == nullptr)
                return;
            m_press_point    = point;
            m_aabb           = get_aabb(m_bbox);
            m_drag_start_deg = m_item->m_transform.deg;

            // compute bbox center in screen coords for rotation
            int bx          = lv_obj_get_x(m_bbox);
            int by          = lv_obj_get_y(m_bbox);
            int bw          = lv_obj_get_width(m_bbox);
            int bh          = lv_obj_get_height(m_bbox);
            m_bbox_center_x = bx + bw / 2;
            m_bbox_center_y = by + bh / 2;

            // 记录按下时矩形中心和尺寸，用于边拖动
            m_press_center_x = (float)(bx + bw / 2);
            m_press_center_y = (float)(by + bh / 2);
            m_press_w        = bw;
            m_press_h        = bh;

            m_is_draging     = true;
        }

        void mouse_move(lv_point_t point) {
            if (!m_is_draging)
                return;

            anchor_mask_t mask = get_current_anchor_mask();

            if (mask == ANCHOR_ROTATE) {
                update_rotation(point);
            } else {
                update_pos(point);
            }
        }

        void mouse_release(lv_point_t point) { m_is_draging = false; }

        void dispatch_event(lv_event_t* e) { m_current_target = (lv_obj_t*)lv_event_get_target(e); }

        void update_rotation(lv_point_t point) {
            if (m_item == nullptr)
                return;

            // angle from center to press point
            float a0 = atan2f((float)(m_press_point.y - m_bbox_center_y), (float)(m_press_point.x - m_bbox_center_x));
            // angle from center to current point
            float a1        = atan2f((float)(point.y - m_bbox_center_y), (float)(point.x - m_bbox_center_x));

            float delta_deg = (a1 - a0) * 180.0f / (float)M_PI;
            m_item->set_rotation(m_drag_start_deg + delta_deg);
            m_item->m_transform.apply(m_bbox);
        }

        void update_pos(lv_point_t point) {
            if (m_item == nullptr)
                return;

            int dx             = point.x - m_press_point.x;
            int dy             = point.y - m_press_point.y;
            anchor_mask_t mask = get_current_anchor_mask();
            float deg          = m_item->m_transform.deg;

            // 无旋转 或 整体移动：走原有逻辑
            if (deg == 0.f || mask == ANCHOR_CENTER) {
                lv_area_t s = solve_rect(m_aabb, mask, dx, dy);
                sync_bbox(s);
                return;
            }

            float rad = deg * (float)M_PI / 180.0f;
            float cs  = cosf(rad);
            float sn  = sinf(rad);

            // Step 1: 鼠标增量投影到局部两轴
            // δ₁ = ΔP · e₁，δ₂ = ΔP · e₂
            float delta1 = (float)dx * cs + (float)dy * sn;
            float delta2 = -(float)dx * sn + (float)dy * cs;

            // Step 2: 根据 anchor_mask 计算原始新宽高
            float new_w = (float)m_press_w;
            float new_h = (float)m_press_h;

            if (mask & ANCHOR_LEFT)
                new_w -= delta1;  // 拖动 AD 边，BC 固定
            if (mask & ANCHOR_RIGHT)
                new_w += delta1;  // 拖动 BC 边，AD 固定
            if (mask & ANCHOR_TOP)
                new_h -= delta2;  // 拖动 AB 边，CD 固定
            if (mask & ANCHOR_BOTTOM)
                new_h += delta2;  // 拖动 CD 边，AB 固定

            // Step 3: 钳位，并反推实际生效的局部分量 δ₁*、δ₂*
            // 钳位后：δ₁* = W - W'（对 LEFT/RIGHT 对称符号相同）
            //         δ₂* = H - H'（对 TOP/BOTTOM 对称符号相同）
            float clamped_w = (new_w < 1.f) ? 1.f : new_w;
            float clamped_h = (new_h < 1.f) ? 1.f : new_h;

            // 实际生效的尺寸变化量（始终为正或零）
            float actual_dw = (float)m_press_w - clamped_w;  // W - W'
            float actual_dh = (float)m_press_h - clamped_h;  // H - H'

            // Step 4: 用实际生效量计算中心偏移，避免固定边漂移
            // 中心偏移 = δ*/2，方向与对应边的操作方向一致
            float center_shift1 = 0.f;  // 沿 e₁ 轴
            float center_shift2 = 0.f;  // 沿 e₂ 轴

            if (mask & ANCHOR_LEFT) {
                // AD 边向 BC 方向移动，中心沿 e₁ 正方向偏移 δ₁*/2
                center_shift1 += actual_dw / 2.f;
            }
            if (mask & ANCHOR_RIGHT) {
                // BC 边向远离 AD 方向移动，中心沿 e₁ 正方向偏移 δ₁*/2
                // actual_dw = W - W'，W' 增大时 actual_dw 为负，符号自洽
                center_shift1 -= actual_dw / 2.f;
            }
            if (mask & ANCHOR_TOP) {
                center_shift2 += actual_dh / 2.f;
            }
            if (mask & ANCHOR_BOTTOM) {
                center_shift2 -= actual_dh / 2.f;
            }

            // Step 5: 局部偏移转回世界坐标
            // O' = O + Δξ·e₁ + Δη·e₂
            float world_shift_x = center_shift1 * cs - center_shift2 * sn;
            float world_shift_y = center_shift1 * sn + center_shift2 * cs;

            float new_cx        = m_press_center_x + world_shift_x;
            float new_cy        = m_press_center_y + world_shift_y;

            // Step 6: 由 O' 反推轴对齐左上角坐标
            int new_x = (int)(new_cx - clamped_w / 2.f);
            int new_y = (int)(new_cy - clamped_h / 2.f);

            // 同步 bbox 和 item，施加旋转变换
            lv_obj_set_pos(m_bbox, new_x, new_y);
            lv_obj_set_size(m_bbox, (int)clamped_w, (int)clamped_h);
            lv_obj_update_layout(m_bbox);

            lv_obj_set_pos(m_item->to_object(), new_x, new_y);
            lv_obj_set_size(m_item->to_object(), (int)clamped_w, (int)clamped_h);

            m_item->m_transform.apply(m_bbox);
            m_item->m_transform.apply(m_item->to_object());
        }

        anchor_mask_t get_current_anchor_mask() {
            if (m_current_target == m_rot)
                return ANCHOR_ROTATE;
            if (m_current_target == m_tm)
                return ANCHOR_TOP;
            if (m_current_target == m_bm)
                return ANCHOR_BOTTOM;
            if (m_current_target == m_lm)
                return ANCHOR_LEFT;
            if (m_current_target == m_rm)
                return ANCHOR_RIGHT;
            if (m_current_target == m_tl)
                return ANCHOR_TOP_LEFT;
            if (m_current_target == m_tr)
                return ANCHOR_TOP_RIGHT;
            if (m_current_target == m_bl)
                return ANCHOR_BOTTOM_LEFT;
            if (m_current_target == m_br)
                return ANCHOR_BOTTOM_RIGHT;
            return ANCHOR_CENTER;
        }

        void sync_bbox(lv_area_t rect) {
            lv_obj_set_pos(m_bbox, rect.x1, rect.y1);
            lv_obj_set_size(m_bbox, rect.x2 - rect.x1, rect.y2 - rect.y1);
            lv_obj_update_layout(m_bbox);
            lv_obj_set_pos(m_item->to_object(), lv_obj_get_x(m_bbox), lv_obj_get_y(m_bbox));
            lv_obj_set_size(m_item->to_object(), lv_obj_get_width(m_bbox), lv_obj_get_height(m_bbox));
        }

        lv_area_t get_aabb(lv_obj_t* object) {
            return {lv_obj_get_x(object), lv_obj_get_y(object), lv_obj_get_x2(object), lv_obj_get_y2(object)};
        }

        bool contains(lv_obj_t* obj) {
            if (m_bbox == obj)
                return true;
            for (auto const& [key, value] : m_handles) {
                if (value.obj == obj)
                    return true;
            }
            return false;
        }

        lv_area_t solve_rect(lv_area_t area, int anchor_mask, int dx, int dy) {
            if (anchor_mask == ANCHOR_CENTER) {
                area.x1 += dx;
                area.x2 += dx;
                area.y1 += dy;
                area.y2 += dy;
                return area;
            }

            if (anchor_mask & ANCHOR_LEFT)
                area.x1 += dx;
            if (anchor_mask & ANCHOR_RIGHT)
                area.x2 += dx;
            if (anchor_mask & ANCHOR_TOP)
                area.y1 += dy;
            if (anchor_mask & ANCHOR_BOTTOM)
                area.y2 += dy;

            if (area.x2 - area.x1 < 1) {
                if (anchor_mask & ANCHOR_LEFT)
                    area.x1 = area.x2 - 1;
                else
                    area.x2 = area.x1 + 1;
            }
            if (area.y2 - area.y1 < 1) {
                if (anchor_mask & ANCHOR_TOP)
                    area.y1 = area.y2 - 1;
                else
                    area.y2 = area.y1 + 1;
            }
            return area;
        }

        lv_obj_t* to_object() { return m_bbox; }

        lv_obj_t* m_bbox     = nullptr;
        GraphicsItem* m_item = nullptr;

        int const k_radius   = 12;
        int const k_diameter = k_radius * 2;

        lv_obj_t* m_tl       = nullptr;
        lv_obj_t* m_tr       = nullptr;
        lv_obj_t* m_bl       = nullptr;
        lv_obj_t* m_br       = nullptr;
        lv_obj_t* m_rot      = nullptr;
        lv_obj_t* m_tm       = nullptr;
        lv_obj_t* m_bm       = nullptr;
        lv_obj_t* m_lm       = nullptr;
        lv_obj_t* m_rm       = nullptr;

        std::map<std::string, Handle> m_handles;

        lv_obj_t* m_current_target = nullptr;
        lv_point_t m_press_point;
        bool m_is_draging = false;
        lv_area_t m_aabb;

        // rotation drag state
        float m_drag_start_deg = 0.f;
        int m_bbox_center_x    = 0;
        int m_bbox_center_y    = 0;

        float m_press_center_x = 0.f;  // 按下时 bbox 中心世界坐标 x
        float m_press_center_y = 0.f;  // 按下时 bbox 中心世界坐标 y
        int m_press_w          = 0;    // 按下时宽度
        int m_press_h          = 0;    // 按下时高度
    };

}  // namespace graphics::v1
