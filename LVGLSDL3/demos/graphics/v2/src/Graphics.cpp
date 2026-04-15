#include "Graphics.h"

#include "GraphicsView.h"
#include "GraphicsScene.h"
#include "GraphicsItem.h"
#include "Gizmos.h"
#include "PropertyPanel.h"
#include "GraphicsObjectItem.h"
#include <vector>

namespace graphics::v2
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

        auto view      = new graphics::v1::GraphicsView(root);
        auto scene     = new graphics::v1::GraphicsScene(view->to_object());
        view->set_scene(scene);

        auto interact_gizmos = new graphics::v1::Gizmos(view->to_object());
        scene->set_interact_gizmos(interact_gizmos);

        auto property_panel = new graphics::v1::PropertyPanel(view->to_object());
        scene->set_property_panel(property_panel);
        lv_obj_align(property_panel->to_object(), LV_ALIGN_RIGHT_MID, -16, 16);

        geom::line_t<> line{
            geom::point_t<>{    0,    0},
            geom::point_t<>{100.f, 80.f}
        };

        geom::polyline_t<> polyline{
            std::vector<geom::point_t<>>{{0, 0}, {70, 70}, {120, 10}, {180, 60}, {240, 10}},
        };

        geom::arc_t<> arc{100, 0, 215};

        geom::triangle_t<> triangle{
            {  0,   0},
            { 90, 100},
            {120,  75}
        };

        geom::circle_t<> circle = {50};

        geom::rect_t<> rect     = {
            {150, 150},
            50,
            100
        };

        geom::polygon_t<> polygon{
            std::vector<geom::point_t<>>{{0}, {0}, {300, 50}, {150, 100}, {120, 75}, {165, 160}, {50, 120}, {0, 0}}
        };

        geom::curve_t<> curve;
        curve.points.emplace_back(geom::bezier_point_t<>{
            {  0,   0},
            {200, 200},
            {350, 150},
        });
        curve.points.emplace_back(geom::bezier_point_t<>{
            {350, 150},
            {250, 300},
            {  0,   0}
        });

        scene->add_item(new GraphicsLineWidget(300, 100, line, scene->to_object()));

        scene->add_item(new GraphicsPolylineWidget(325, 125, polyline, scene->to_object()));

        scene->add_item(new GraphicsArcWidget(325, 125, arc, scene->to_object()));

        scene->add_item(new GraphicsTriangleWidget(325, 125, triangle, scene->to_object()));

        scene->add_item(new GraphicsCircleWidget(325, 125, circle, scene->to_object()));

        scene->add_item(new GraphicsRectWidget(325, 125, rect, scene->to_object()));

        scene->add_item(new GraphicsPolygonWidget(325, 125, polygon, scene->to_object()));

        scene->add_item(new GraphicsCurveWidget(325, 125, curve, scene->to_object()));

        scene->add_item(
            new GraphicsTextWidget(325, 125, "LVGL sdl3 graphics v2 demos", lv_font_get_default(), scene->to_object()));

        scene->add_item(new GraphicsImageWidget(325, 125, R"(Z:/home/logo.bmp)", scene->to_object()));

        scene->add_item(new GraphicsImageWidget(325, 125, R"(Z:/home/logo.png)", scene->to_object()));

        lv_obj_update_layout(root);
    }
}  // namespace graphics::v2
