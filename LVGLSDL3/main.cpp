#include "main.h"
#include <SDL3/SDL.h>
#include <print>

#include "Graphics.h"

template <typename F>
static lv_obj_t* create_button(lv_obj_t* parent, char const* text, F f, void* user_data) {
    lv_obj_t* btn = lv_button_create(parent);
    lv_obj_add_event_cb(btn, f, LV_EVENT_CLICKED, user_data);

    lv_obj_t* label = lv_label_create(btn);
    lv_label_set_text(label, text);
    lv_obj_center(label);

    return btn;
}

#ifdef ENABLE_GRAPHICS_V1_DEMOS
using namespace graphics::v1;
#endif

#ifdef ENABLE_GRAPHICS_V2_DEMOS
using namespace graphics::v2;
#endif

#ifdef ENABLE_GRAPHICS_V3_DEMOS
using namespace graphics::v3;
#endif

int main(int argc, char* argv[]) {
    LVGLSDL3 sdl(1024, 600);

    create_graphics_demo();

    lv_obj_t* close_btn = create_button(
        lv_layer_top(),
        "X",
        [](auto e)
        {
            LVGLSDL3* lvgl_sdl3 = (LVGLSDL3*)lv_event_get_user_data(e);
            lvgl_sdl3->stop();
        },
        &sdl);

    lv_obj_set_size(close_btn, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_radius(close_btn, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_bg_color(close_btn, lv_palette_main(LV_PALETTE_RED), LV_PART_MAIN);
    lv_obj_align(close_btn, LV_ALIGN_TOP_RIGHT, -6, 6);

    sdl.run();
    return 0;
}
