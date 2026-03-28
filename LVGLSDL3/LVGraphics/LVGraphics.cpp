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
    LVGraphicsItem() = default;
    virtual ~LVGraphicsItem() = default;
    lv_obj_t* to_object() { return m_obj; }
    lv_obj_t* m_obj;
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


class LVGraphicsInteractItem
{
public:
    LVGraphicsInteractItem()
    {
        m_box = lv_obj_create(lv_scr_act());
        lv_obj_set_scroll_dir(m_box, LV_DIR_NONE);
        lv_obj_set_size(m_box, 1, 1);
        lv_obj_set_style_pad_all(m_box, 0, 0);
        lv_obj_set_style_margin_all(m_box, 0, 0);

        lv_obj_set_style_border_color(m_box, lv_color_make(255, 0, 0), 0);
        lv_obj_set_style_border_width(m_box, 1, 0);
    }
    void selected(LVGraphicsItem* item)
    {
        m_item = item;

        int x = lv_obj_get_x(item->to_object());
        int y = lv_obj_get_y(item->to_object());
        lv_obj_set_pos(m_box, x, y);
        lv_obj_set_size(m_box, lv_obj_get_width(item->to_object()), lv_obj_get_height(item->to_object()));
    }

    lv_obj_t* m_box;
    LVGraphicsItem* m_item = nullptr;
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
        if (ok)
        {
            m_interact_item.selected(item);
        }
    }

    void mouse_move(lv_event_t* e, lv_point_t point) {}

    void mouse_release(lv_event_t* e, lv_point_t point) {}

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

    std::vector<LVGraphicsItem*> m_items;

    lv_obj_t* m_obj;

    Transform m_transform;

    LVGraphicsInteractItem m_interact_item;
    lv_point_t m_last_point;
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
        std::println("mouse_press: {} {}", point.x, point.y);
        if (m_scene)
            m_scene->mouse_press(e, point);
    }

    void mouse_move(lv_event_t* e, lv_point_t point)
    {
        std::println("mouse_move: {} {}", point.x, point.y);
        if (m_scene)
            m_scene->mouse_move(e, point);
    }

    void mouse_release(lv_event_t* e, lv_point_t point)
    {
        std::println("mouse_release: {} {}", point.x, point.y);
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

    scene->add_item(new LVGraphicsRectItem(50, 50, 100, 200, scene->to_object()));
    scene->add_item(new LVGraphicsRectItem(200, 50, 100, 50, scene->to_object()));
}
