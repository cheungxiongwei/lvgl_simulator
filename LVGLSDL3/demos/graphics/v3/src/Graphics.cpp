#include "Graphics.h"
#include "VectorCanvas.h"

namespace graphics::v3
{
    void create_graphics_demo() {
        lv_obj_t* root = lv_screen_active();
        auto canvas    = new VectorCanvas(root);

        lv_obj_add_event_cb(
            root,
            [](auto e)
            {
                VectorCanvas* canvas = (VectorCanvas*)lv_event_get_user_data(e);
                if (canvas) {
                    delete canvas;
                }
            },
            LV_EVENT_DELETE,
            canvas);
    }

}  // namespace graphics::v3
