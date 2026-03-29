#include "LVGraphics.h"
#include <cstdio>
#include <ostream>
#include <vector>

struct Transform
{
    float tx = 0.f;
    float ty = 0.f;
    float sx = 1.f;
    float sy = 1.f;
    float deg = 0.f;

    void apply(lv_obj_t* obj)
    {
        if (obj == nullptr)
            return;

        float cx = static_cast<float>(lv_obj_get_width(obj) / 2);
        float cy = static_cast<float>(lv_obj_get_height(obj) / 2);

        lv_matrix_t matrix;
        lv_matrix_identity(&matrix);
        lv_matrix_translate(&matrix, cx, cy);
        lv_matrix_rotate(&matrix, deg);
        lv_matrix_scale(&matrix, sx, sy);
        lv_matrix_translate(&matrix, -cx, -cy);
        lv_matrix_translate(&matrix, tx, ty);
        lv_obj_set_transform(obj, &matrix);
    }
};

class LVGraphicsItem
{
public:
    void rotate(float deg)
    {
        m_transform.deg += deg;
        while (m_transform.deg >= 360)
            m_transform.deg -= 360;

        m_transform.apply(m_obj);
    }


    LVGraphicsItem() = default;
    virtual ~LVGraphicsItem() = default;
    lv_obj_t* to_object() { return m_obj; }
    lv_obj_t* m_obj = nullptr;
    Transform m_transform;
};

class LVGraphicsRectItem : public LVGraphicsItem
{
public:
    LVGraphicsRectItem(int x, int y, int width, int height, lv_obj_t* parent)
    {
        m_obj = lv_obj_create(parent);
        lv_obj_set_scroll_dir(m_obj, LV_DIR_NONE);
        lv_obj_set_pos(m_obj, x, y);
        lv_obj_set_size(m_obj, width, height);
        lv_obj_set_flag(m_obj, LV_OBJ_FLAG_EVENT_BUBBLE, true);
        lv_obj_set_style_pad_all(m_obj, 0, 0);
        lv_obj_set_style_margin_all(m_obj, 0, 0);

        lv_obj_set_style_border_color(m_obj, lv_color_make(0, 0, 255), 0);
        lv_obj_set_style_border_width(m_obj, 1, 0);
    }
};


class LVGraphicsInteractGizmos
{
public:
    LVGraphicsInteractGizmos(lv_obj_t* parent)
    {
        m_box = lv_obj_create(parent);
        lv_obj_set_scroll_dir(m_box, LV_DIR_NONE);
        lv_obj_set_flag(m_box, LV_OBJ_FLAG_EVENT_BUBBLE, true);
        lv_obj_set_size(m_box, 1, 1);
        lv_obj_set_style_bg_opa(m_box, LV_OPA_TRANSP, 0);
        lv_obj_set_style_pad_all(m_box, 0, 0);
        lv_obj_set_style_margin_all(m_box, 0, 0);

        lv_obj_set_style_border_color(m_box, lv_color_make(255, 0, 0), 0);
        lv_obj_set_style_border_width(m_box, 6, 0);

        create_handle(m_tl, LV_ALIGN_TOP_LEFT);
        create_handle(m_tr, LV_ALIGN_TOP_RIGHT);
        create_handle(m_bl, LV_ALIGN_BOTTOM_LEFT);
        create_handle(m_br, LV_ALIGN_BOTTOM_RIGHT);
        create_handle(m_rot, LV_ALIGN_TOP_MID);
    }

    void create_handle(lv_obj_t* handle, lv_align_t align)
    {
        handle = lv_button_create(m_box);
        lv_obj_set_size(handle, 32, 32);
        lv_obj_align(handle, align, 0, 0);
    }

    void set_visible(bool visible) { lv_obj_set_flag(m_box, LV_OBJ_FLAG_HIDDEN, !visible); }

    void selected(LVGraphicsItem* item)
    {
        m_item = item;
        if (m_item == nullptr)
        {
            set_visible(false);
            return;
        }
        set_visible(true);

        int x = lv_obj_get_x(item->to_object());
        int y = lv_obj_get_y(item->to_object());
        int w = lv_obj_get_width(item->to_object());
        int h = lv_obj_get_height(item->to_object());

        lv_obj_set_pos(m_box, x, y);
        lv_obj_set_size(m_box, w, h);
        lv_obj_update_layout(m_box);

        m_item->m_transform.apply(m_box);
    }

    bool has_selected() { return m_item != nullptr; }

    void mouse_press(lv_point_t point)
    {
        if (m_item == nullptr)
            return;

        int x = lv_obj_get_x(m_box);
        int y = lv_obj_get_y(m_box);

        m_delta_point.x = point.x - x;
        m_delta_point.y = point.y - y;

        update_pos(point);
    }

    void mouse_move(lv_point_t point) { update_pos(point); }

    void mouse_release(lv_point_t point) { update_pos(point); }

    void update_pos(lv_point_t point)
    {
        if (m_item == nullptr)
            return;

        lv_obj_set_pos(m_box, point.x - m_delta_point.x, point.y - m_delta_point.y);
        lv_obj_update_layout(m_box);
        lv_obj_set_pos(m_item->to_object(), lv_obj_get_x(m_box), lv_obj_get_y(m_box));
        lv_obj_set_size(m_item->to_object(), lv_obj_get_width(m_box), lv_obj_get_height(m_box));
    }

    lv_obj_t* to_object() { return m_box; }

    lv_obj_t* m_box;
    LVGraphicsItem* m_item = nullptr;
    lv_point_t m_delta_point;

    lv_obj_t* m_tl;
    lv_obj_t* m_tr;
    lv_obj_t* m_bl;
    lv_obj_t* m_br;
    lv_obj_t* m_rot;
};

class LVGraphicsScene
{
public:
    LVGraphicsScene(lv_obj_t* parent)
    {
        m_obj = lv_obj_create(parent);
        lv_obj_set_scroll_dir(m_obj, LV_DIR_NONE);
        lv_obj_set_size(m_obj, LV_PCT(100), LV_PCT(100));
        lv_obj_set_flag(m_obj, LV_OBJ_FLAG_EVENT_BUBBLE, true);
        lv_obj_set_style_pad_all(m_obj, 0, 0);
        lv_obj_set_style_margin_all(m_obj, 0, 0);

        lv_obj_set_style_border_color(m_obj, lv_color_make(0, 255, 0), 0);
        lv_obj_set_style_border_width(m_obj, 1, 0);
    }
    void mouse_press(lv_event_t* e, lv_point_t point)
    {
        lv_obj_t* active = lv_indev_get_active_obj();
        auto [ok, item] = contains(active);

        if (active != m_interact_item->to_object())
            m_interact_item->selected(ok ? item : nullptr);
        m_interact_item->mouse_press(point);
    }

    void mouse_move(lv_event_t* e, lv_point_t point)
    {
        std::println("mouse_move: {} {} {}", point.x, point.y, m_interact_item->has_selected());
        if (m_interact_item->has_selected())
        {
            m_interact_item->mouse_move(point);
        }
    }

    void mouse_release(lv_event_t* e, lv_point_t point)
    {
        if (m_interact_item->has_selected())
        {
            m_interact_item->mouse_release(point);
        }
    }

    void add_item(LVGraphicsItem* item) { m_items.emplace_back(item); }

    lv_obj_t* to_object() { return m_obj; }

    std::pair<bool, LVGraphicsItem*> contains(lv_obj_t* obj)
    {
        for (auto item : m_items)
        {
            if (item->m_obj == obj)
                return {true, item};
        }
        return {false, nullptr};
    }

    void set_interact_item(LVGraphicsInteractGizmos* interact_item) { m_interact_item = interact_item; }


    std::vector<LVGraphicsItem*> m_items;

    lv_obj_t* m_obj;

    Transform m_transform;

    LVGraphicsInteractGizmos* m_interact_item;
};

class LVGraphicsView
{
public:
    LVGraphicsView(lv_obj_t* parent)
    {
        m_obj = lv_obj_create(parent);
        lv_obj_set_scroll_dir(m_obj, LV_DIR_NONE);
        lv_obj_set_size(m_obj, LV_PCT(100), LV_PCT(100));
        lv_obj_set_style_pad_all(m_obj, 0, 0);
        lv_obj_set_style_margin_all(m_obj, 0, 0);

        lv_obj_set_style_border_color(m_obj, lv_color_make(255, 0, 0), 0);
        lv_obj_set_style_border_width(m_obj, 1, 0);

        lv_obj_add_event_cb(
            m_obj,
            [](lv_event_t* e)
            {
                LVGraphicsView* view = (LVGraphicsView*)lv_event_get_user_data(e);
                lv_event_code_t code = lv_event_get_code(e);

                lv_indev_t* indev = lv_indev_active();
                if (indev == nullptr)
                    return;

                lv_point_t point;
                lv_indev_get_point(indev, &point);


                if (code == LV_EVENT_PRESSED)
                {
                    view->mouse_press(e, point);
                }
                else if (code == LV_EVENT_RELEASED)
                {
                    view->mouse_release(e, point);
                }
                else if (code == LV_EVENT_PRESSING)
                {
                    view->mouse_move(e, point);
                }
            },
            LV_EVENT_ALL, this);
    }

    void mouse_press(lv_event_t* e, lv_point_t point)
    {

        if (m_scene)
            m_scene->mouse_press(e, point);
    }

    void mouse_move(lv_event_t* e, lv_point_t point)
    {

        if (m_scene)
            m_scene->mouse_move(e, point);
    }

    void mouse_release(lv_event_t* e, lv_point_t point)
    {
        if (m_scene)
            m_scene->mouse_release(e, point);
    }

    void set_scene(LVGraphicsScene* scene) { m_scene = scene; }

    lv_obj_t* to_object() { return m_obj; }

    lv_obj_t* m_obj;
    Transform m_transform;

    LVGraphicsScene* m_scene;
};

void CreateLVGraphics()
{
    auto view = new LVGraphicsView(lv_screen_active());
    auto scene = new LVGraphicsScene(view->to_object());
    view->set_scene(scene);

    LVGraphicsInteractGizmos* interact_item = new LVGraphicsInteractGizmos(view->to_object());
    scene->set_interact_item(interact_item);

    auto item1 = new LVGraphicsRectItem(150, 200, 200, 300, scene->to_object());
    scene->add_item(item1);
    scene->add_item(new LVGraphicsRectItem(50, 50, 100, 200, scene->to_object()));
    scene->add_item(new LVGraphicsRectItem(200, 50, 100, 50, scene->to_object()));
    scene->add_item(new LVGraphicsRectItem(500, 400, 100, 50, scene->to_object()));

    lv_obj_update_layout(lv_screen_active());
    item1->rotate(45);
}
