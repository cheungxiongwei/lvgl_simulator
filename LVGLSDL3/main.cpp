#include "main.h"
#include <SDL3/SDL.h>
#include <iostream>

int main(int argc, char* argv[]) {
    LVGLSDL3 sdl(480, 320);

    // lv_theme_t* theme = lv_theme_default_init(NULL, lv_color_make(255, 255, 255), lv_color_make(255, 0, 0), true, LV_FONT_DEFAULT);
    // lv_theme_t* theme = lv_theme_mono_init(NULL, true, LV_FONT_DEFAULT);
    lv_theme_t* theme = lv_theme_simple_init(NULL);
    lv_disp_set_theme(NULL, theme);

    // Create a simple UI
    lv_obj_t* btn = lv_btn_create(lv_screen_active());
    lv_obj_set_size(btn, 120, 50);
    lv_obj_align(btn, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(btn, [](auto e) { std::cout << "aa" << std::endl; }, LV_EVENT_CLICKED, 0);

    lv_obj_t* label = lv_label_create(btn);
    lv_label_set_text(label, "Hello World");
    lv_obj_center(label);

    sdl.run();
    return 0;
}
