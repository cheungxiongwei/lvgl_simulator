#include "LVGraphics.h"
#include <cmath>

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

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

struct Transform {
    float tx = 0.f;
    float ty = 0.f;
    float sx = 1.f;
    float sy = 1.f;
    // --- 旋转（角度制，顺时针为正，与 LVGL 一致）---
    float deg     = 0.f;
    float pivot_x = 0.5f;
    float pivot_y = 0.5f;

    lv_matrix_t build(lv_obj_t const* obj) const {
        float w  = static_cast<float>(lv_obj_get_width(const_cast<lv_obj_t*>(obj)));
        float h  = static_cast<float>(lv_obj_get_height(const_cast<lv_obj_t*>(obj)));

        float cx = w * pivot_x;
        float cy = h * pivot_y;

        lv_matrix_t m;
        lv_matrix_identity(&m);
        lv_matrix_translate(&m, tx, ty);
        lv_matrix_translate(&m, cx, cy);
        lv_matrix_rotate(&m, deg);
        lv_matrix_scale(&m, sx, sy);
        lv_matrix_translate(&m, -cx, -cy);

        return m;
    }

    Transform& apply(lv_obj_t* obj) {
        if (obj == nullptr)
            return *this;
        lv_matrix_t m = build(obj);
        lv_obj_set_transform(obj, &m);
        return *this;
    }

    Transform& reset() {
        tx = ty = 0.f;
        sx = sy = 1.f;
        deg     = 0.f;
        pivot_x = pivot_y = 0.5f;
        return *this;
    }

    static Transform from_translate(float x, float y) {
        Transform t;
        t.tx = x;
        t.ty = y;
        return t;
    }

    static Transform from_rotate(float deg) {
        Transform t;
        t.deg = deg;
        return t;
    }

    static Transform from_scale(float s) {
        Transform t;
        t.sx = t.sy = s;
        return t;
    }
};

class LVGraphicsItem
{
public:
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

    LVGraphicsItem() = default;
    virtual ~LVGraphicsItem() {};

    lv_obj_t* to_object() { return m_obj; }

    Transform& transform() { return m_transform; }

    lv_obj_t* m_obj = nullptr;
    Transform m_transform;
};

class LVGraphicsPropertyPanel
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

    LVGraphicsPropertyPanel(lv_obj_t* parent) : m_ref_item(nullptr) {
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

    ~LVGraphicsPropertyPanel() { lv_obj_delete(m_layout); }

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

        lv_obj_add_event_cb(field.input, LVGraphicsPropertyPanel::event, LV_EVENT_READY, this);
        m_properties[name] = field;
        return field;
    }

    void show_for_item(LVGraphicsItem* item) {
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
        LVGraphicsPropertyPanel* self = (LVGraphicsPropertyPanel*)lv_event_get_user_data(e);
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

    LVGraphicsItem* m_ref_item = nullptr;
    lv_obj_t* m_layout         = nullptr;
    std::map<std::string, PropertyField> m_properties;
};

class LVGraphicsRectItem : public LVGraphicsItem
{
public:
    LVGraphicsRectItem(int x, int y, int width, int height, lv_obj_t* parent) {
        m_obj = lv_obj_create(parent);
        lv_obj_set_scroll_dir(m_obj, LV_DIR_NONE);
        lv_obj_set_pos(m_obj, x, y);
        lv_obj_set_size(m_obj, width, height);
        lv_obj_set_flag(m_obj, LV_OBJ_FLAG_EVENT_BUBBLE, true);
        lv_obj_set_style_pad_all(m_obj, 0, LV_PART_MAIN);
        lv_obj_set_style_margin_all(m_obj, 0, LV_PART_MAIN);
        lv_obj_set_style_border_color(m_obj, lv_palette_main(LV_PALETTE_GREY), LV_PART_MAIN);
        lv_obj_set_style_border_width(m_obj, 1, LV_PART_MAIN);
    }
};

class LVGraphicsInteractGizmos
{
public:
    struct Handle {
        lv_obj_t* obj;
        lv_align_t align;
        int x_offset;
        int y_offset;
    };

    LVGraphicsInteractGizmos(lv_obj_t* parent) {
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
            [](lv_event_t* e) { ((LVGraphicsInteractGizmos*)lv_event_get_user_data(e))->dispatch_event(e); },
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
            [](lv_event_t* e) { ((LVGraphicsInteractGizmos*)lv_event_get_user_data(e))->dispatch_event(e); },
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

    void selected(LVGraphicsItem* item) {
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

    LVGraphicsItem* current_selected_item() { return m_item; }

    void mouse_press(lv_point_t point) {
        if (m_item == nullptr)
            return;
        m_press_point    = point;
        m_aabb           = get_aabb(m_bbox);
        m_is_draging     = true;
        m_drag_start_deg = m_item->m_transform.deg;

        // compute bbox center in screen coords for rotation
        int bx          = lv_obj_get_x(m_bbox);
        int by          = lv_obj_get_y(m_bbox);
        int bw          = lv_obj_get_width(m_bbox);
        int bh          = lv_obj_get_height(m_bbox);
        m_bbox_center_x = bx + bw / 2;
        m_bbox_center_y = by + bh / 2;

           // 新增：记录按下时矩形中心和尺寸，用于边拖动
        m_press_center_x = (float)(bx + bw / 2);
        m_press_center_y = (float)(by + bh / 2);
        m_press_w        = bw;
        m_press_h        = bh;
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

        lv_obj_set_pos(m_item->to_object(), new_x, new_y);
        lv_obj_set_size(m_item->to_object(), (int)clamped_w, (int)clamped_h);

        lv_obj_update_layout(m_bbox);

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

    lv_obj_t* to_object() { return m_bbox; }

    lv_obj_t* m_bbox       = nullptr;
    LVGraphicsItem* m_item = nullptr;

    int const k_radius     = 12;
    int const k_diameter   = k_radius * 2;

    lv_obj_t* m_tl         = nullptr;
    lv_obj_t* m_tr         = nullptr;
    lv_obj_t* m_bl         = nullptr;
    lv_obj_t* m_br         = nullptr;
    lv_obj_t* m_rot        = nullptr;
    lv_obj_t* m_tm         = nullptr;
    lv_obj_t* m_bm         = nullptr;
    lv_obj_t* m_lm         = nullptr;
    lv_obj_t* m_rm         = nullptr;

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

class LVGraphicsScene
{
public:
    LVGraphicsScene(lv_obj_t* parent) {
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

    void add_item(LVGraphicsItem* item) { m_items.emplace_back(item); }

    lv_obj_t* to_object() { return m_obj; }

    std::pair<bool, LVGraphicsItem*> contains(lv_obj_t* obj) {
        for (auto item : m_items) {
            if (item->m_obj == obj)
                return {true, item};
        }
        return {false, nullptr};
    }

    void set_interact_gizmos(LVGraphicsInteractGizmos* item) { m_interact_gizmos = item; }

    LVGraphicsInteractGizmos* get_interact_gizmos() { return m_interact_gizmos; }

    void set_property_panel(LVGraphicsPropertyPanel* item) { m_property_panel = item; }

    LVGraphicsPropertyPanel* get_property_panel() { return m_property_panel; }

    lv_obj_t* m_obj = nullptr;
    Transform m_transform;
    std::vector<LVGraphicsItem*> m_items;
    LVGraphicsInteractGizmos* m_interact_gizmos = nullptr;
    LVGraphicsPropertyPanel* m_property_panel   = nullptr;
};

class LVGraphicsView
{
public:
    LVGraphicsView(lv_obj_t* parent) {
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

                LVGraphicsView* view = (LVGraphicsView*)lv_event_get_user_data(e);
                lv_indev_t* indev    = lv_indev_active();
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

    void set_scene(LVGraphicsScene* scene) { m_scene = scene; }

    lv_obj_t* to_object() { return m_obj; }
private:
    lv_obj_t* m_obj = nullptr;
    Transform m_transform;
    LVGraphicsScene* m_scene = nullptr;
};

void CreateLVGraphics() {
    lv_obj_t* root = lv_screen_active();

    auto view      = new LVGraphicsView(root);
    auto scene     = new LVGraphicsScene(view->to_object());
    view->set_scene(scene);

    auto interact_gizmos = new LVGraphicsInteractGizmos(view->to_object());
    scene->set_interact_gizmos(interact_gizmos);

    auto property_panel = new LVGraphicsPropertyPanel(view->to_object());
    scene->set_property_panel(property_panel);
    lv_obj_align(property_panel->to_object(), LV_ALIGN_RIGHT_MID, -16, 16);

    auto item1 = new LVGraphicsRectItem(150, 200, 200, 300, scene->to_object());
    scene->add_item(item1);
    scene->add_item(new LVGraphicsRectItem(50, 50, 100, 200, scene->to_object()));
    scene->add_item(new LVGraphicsRectItem(200, 50, 100, 50, scene->to_object()));
    scene->add_item(new LVGraphicsRectItem(500, 400, 100, 50, scene->to_object()));

    lv_obj_update_layout(root);

    item1->rotate(45);
}
