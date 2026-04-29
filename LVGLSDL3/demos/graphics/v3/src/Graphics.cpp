#include "Graphics.h"
#include "VectorCanvas.h"

namespace graphics::v3
{
    void create_graphics_demo() {
        auto layout = Layout(Layout::Dir::Row, lv_screen_active());
        lv_obj_set_flex_align(layout.object(), LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_scroll_dir(layout.object(), LV_DIR_NONE);
        lv_obj_set_size(layout.object(), LV_PCT(100), LV_PCT(100));
        lv_obj_set_style_margin_all(layout.object(), 0, LV_PART_MAIN);
        lv_obj_set_style_pad_all(layout.object(), 0, LV_PART_MAIN);

        auto layerWidget = new LayerListWidget(layout.object());

        auto canvas      = new VectorCanvas(layout.object());
        auto toolbar     = new Toolbar(canvas->object());
        auto statusbar   = new StatusBar(canvas->object());
        auto zoomWidget  = new ZoomWidget(canvas->object());

        canvas->setToolBar(toolbar);
        canvas->setStatusBar(statusbar);
        canvas->setZoomWidget(zoomWidget);

        layerWidget->setShapes(canvas->getShapes(), canvas);
        canvas->notify.push_back(std::bind(&LayerListWidget::shapesChanged, layerWidget));
    }

}  // namespace graphics::v3
