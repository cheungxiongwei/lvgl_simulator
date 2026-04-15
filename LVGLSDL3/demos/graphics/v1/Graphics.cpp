#include "Graphics.h"

#include "GraphicsView.h"
#include "GraphicsScene.h"
#include "GraphicsItem.h"
#include "Gizmos.h"
#include "PropertyPanel.h"

namespace graphics::v1
{
    void create_graphics_demo() {
        lv_point_precise_t list1[] = {
            {  5,  5},
            { 70, 70},
            {120, 10},
            {180, 60},
            {240, 10}
        };

        lv_point_precise_t list2[] = {
            {150,  50},
            { 35, 250},
            {265, 250},
        };

        lv_point_precise_t list3[] = {
            { 20, 30},
            {100, 80},
        };

        lv_obj_t* root = lv_screen_active();

        auto view      = new GraphicsView(root);
        auto scene     = new GraphicsScene(view->to_object());
        view->set_scene(scene);

        auto interact_gizmos = new Gizmos(view->to_object());
        scene->set_interact_gizmos(interact_gizmos);

        auto property_panel = new PropertyPanel(view->to_object());
        scene->set_property_panel(property_panel);
        lv_obj_align(property_panel->to_object(), LV_ALIGN_RIGHT_MID, -16, 16);

        auto item1 = new GraphicsRectItem(150, 200, 200, 300, scene->to_object());
        lv_obj_update_layout(item1->to_object());
        item1->rotate(45);
        scene->add_item(item1);

        scene->add_item(new GraphicsRectItem(50, 50, 100, 200, scene->to_object()));
        scene->add_item(new GraphicsRectItem(200, 50, 100, 50, scene->to_object()));

        scene->add_item(new GraphicsTextItem(500, 400, "LVGL sdl3 graphics demos!", scene->to_object()));

        scene->add_item(new GraphicsPolygonItem(45, 45, list1, false, scene->to_object()));
        scene->add_item(new GraphicsPolygonItem(50, 50, list2, true, scene->to_object()));
        scene->add_item(new GraphicsPolygonItem(180, 180, list3, true, scene->to_object()));
    }
}  // namespace graphics::v1
