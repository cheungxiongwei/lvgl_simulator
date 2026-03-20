#include "main.h"
#include <SDL3/SDL.h>
#include <print>

template <typename F>
static lv_obj_t* create_button(lv_obj_t* parent, char const* text, F f, void* user_data) {
    lv_obj_t* btn = lv_button_create(parent);
    lv_obj_add_event_cb(btn, f, LV_EVENT_CLICKED, user_data);

    lv_obj_t* label = lv_label_create(btn);
    lv_label_set_text(label, text);
    lv_obj_center(label);

    return btn;
}

int main(int argc, char* argv[]) {
    LVGLSDL3 sdl(480, 320);

    // lv_theme_t* theme = lv_theme_default_init(NULL, lv_color_make(255, 255, 255), lv_color_make(255, 0, 0), true, LV_FONT_DEFAULT);
    // lv_theme_t* theme = lv_theme_mono_init(NULL, true, LV_FONT_DEFAULT);
    lv_theme_t* theme = lv_theme_simple_init(NULL);
    lv_disp_set_theme(NULL, theme);

    // Create a simple UI
    lv_obj_t* root = lv_screen_active();
    lv_obj_t* hello_world_btn =
        create_button(root, "Hello World", [](auto e) { std::cout << "clicked" << std::endl; }, NULL);
    lv_obj_center(hello_world_btn);

    lv_obj_t* close_btn = create_button(
        root,
        "Close",
        [](auto e)
        {
            LVGLSDL3* lvgl_sdl3 = (LVGLSDL3*)lv_event_get_user_data(e);
            lvgl_sdl3->stop();
        },
        &sdl);
    lv_obj_align(close_btn, LV_ALIGN_TOP_RIGHT, -6, 6);

    lv_async_call(
        [](void* user_data)
        {
            auto const button = static_cast<lv_obj_t*>(user_data);
            lv_area_t coord;
            lv_obj_get_coords(button, &coord);
            printf("%d %d %d %d\n", coord.x1, coord.y1, coord.x2, coord.y2);
            fflush(stdout);
        },
        hello_world_btn);

    sdl.run();
    return 0;
}
