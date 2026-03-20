#pragma once

#include <QWidget>
#include <QPainter>
#include <QImage>
#include <QMouseEvent>

#include <chrono>

#include <lvgl.h>
#include <lvgl/src/display/lv_display_private.h>

// LVGL 模拟器
class LVGLWidget : public QWidget
{
    Q_OBJECT

public:
    LVGLWidget(int width = 640, int height = 480, QWidget *parent = nullptr)
      : QWidget(parent)
      , hor_res(width)
      , ver_res(height)
      , image(hor_res, ver_res, QImage::Format_ARGB32) {
        setMouseTracking(true);
        setWindowTitle(QString("LVGL Simulator %1x%2").arg(hor_res).arg(ver_res));
        setFixedSize(hor_res, ver_res);

        // [1] 内部资源
        // 初始化 LVGL 库（必须最先调用）
        // 此函数会初始化内部的内存管理、任务管理、对象系统等核心模块
        lv_init();

        // 初始化自己的其他外设驱动
        // TODO

        // [2] 系统时钟
        // 初始化系统时钟滴答 (tick)
        // 如果你使用手动模式，请周期性调用 lv_tick_inc(ms) 函数，通知 LVGL 已经过了多少毫秒。该函数应在独立线程或定时器中调用，参数 ms 必须等于实际的时间间隔（单位：毫秒），以确保 LVGL 的计时准确。
        lv_tick_set_cb([]() -> uint32_t {
            // 自系统启动的毫秒数
            auto now = std::chrono::steady_clock::now();
            auto ms  = std::chrono::duration_cast<std::chrono::milliseconds>(now - system_start).count();
            return static_cast<uint32_t>(ms);
        });

        // [3] 显示
        // 初始化显示（LVGL 显示驱动配置）
        display = lv_display_create(hor_res, ver_res);
        Q_CHECK_PTR(display);
        lv_display_set_user_data(display, this);
        lv_display_set_antialiasing(display, true);
        lv_display_set_buffers(display, image.bits(), NULL, image.sizeInBytes(), LV_DISPLAY_RENDER_MODE_FULL);
        lv_display_set_flush_cb(display, [](lv_display_t *display, const lv_area_t *, uint8_t *) {
            // 由于我们选择的渲染模式是 LV_DISPLAY_RENDER_MODE_FULL，所以这里不需要拷贝内存。

            /* IMPORTANT!!! 通知 LVGL 刷新完成 */
            lv_display_flush_ready(display);
        });

        // [4] 输入
        // 初始化输入（LVGL 输入驱动配置）
        // 鼠标
        mouse_indev = lv_indev_create();
        Q_CHECK_PTR(mouse_indev);
        lv_indev_set_user_data(mouse_indev, this);
        lv_indev_set_type(mouse_indev, LV_INDEV_TYPE_POINTER);
        lv_indev_set_read_cb(mouse_indev, [](lv_indev_t *indev, lv_indev_data_t *data) {
            auto self = static_cast<LVGLWidget *>(lv_indev_get_user_data(indev));
            Q_CHECK_PTR(self);
            data->state = self->mouse_state;
            data->point = self->mouse_point;
        });

        key_indev = lv_indev_create();
        Q_CHECK_PTR(key_indev);
        lv_indev_set_user_data(key_indev, this);
        lv_indev_set_type(key_indev, LV_INDEV_TYPE_KEYPAD);
        lv_indev_set_read_cb(key_indev, [](lv_indev_t *indev, lv_indev_data_t *data) {
            auto self = static_cast<LVGLWidget *>(lv_indev_get_user_data(indev));
            Q_CHECK_PTR(self);
            data->state = self->key_state;
            data->key   = self->key_code;
        });

        // [5] 定时器
        // 初始化定时器,每隔 40ms 调用 lv_timer_handler()，驱动 LVGL 内部任务（动画/输入/刷新等）。
        using namespace std::chrono_literals;
        startTimer(40ms);  // 40ms(25Hz) 16ms(约60Hz)

        // 设置背景颜色
        lv_obj_set_style_bg_color(lv_screen_active(), lv_color_make(255, 255, 255), LV_PART_MAIN);

        lvgl_example();
    }

    ~LVGLWidget() {
        // 清理资源
        if(display) {
            lv_display_delete(display);
            display = nullptr;
        }

        if(mouse_indev) {
            lv_indev_delete(mouse_indev);
            mouse_indev = nullptr;
        }

        if(key_indev) {
            lv_indev_delete(key_indev);
            key_indev = nullptr;
        }

        // 清理 LVGL 资源
        // 注意：如果有多个 LVGL 实例，只应在最后一个实例销毁时调用
        lv_deinit();
    }

    QSize sizeHint() const override { return {hor_res, ver_res}; }

    // 对于不同演示，继承 LVGLWidget 实现该方法
    virtual void lvgl_example() {
        // [7] 创建一个按钮示例
        lv_obj_t *obj = lv_button_create(lv_screen_active());
        Q_CHECK_PTR(obj);
        lv_obj_set_size(obj, 100, 50);
        lv_obj_set_pos(obj, 50, 50);
        lv_obj_add_event(obj, [](lv_event_t *e) { qDebug() << "clicked cheungxiongwei"; }, LV_EVENT_CLICKED, this);
        lv_obj_center(obj);

        // 添加标签
        lv_obj_t *label = lv_label_create(obj);
        Q_CHECK_PTR(label);
        lv_label_set_text(label, "Button");
        lv_obj_center(label);
    }

protected:
    void timerEvent(QTimerEvent *event) override {
        lv_timer_handler();
        update();
    }

    void mousePressEvent(QMouseEvent *event) override {
        mouse_state   = LV_INDEV_STATE_PRESSED;
        mouse_point.x = event->pos().x();
        mouse_point.y = event->pos().y();
        QWidget::mousePressEvent(event);
    }

    void mouseReleaseEvent(QMouseEvent *event) override {
        mouse_state   = LV_INDEV_STATE_RELEASED;
        mouse_point.x = event->pos().x();
        mouse_point.y = event->pos().y();
        QWidget::mouseReleaseEvent(event);
    }

    void mouseMoveEvent(QMouseEvent *event) override {
        mouse_point.x = event->pos().x();
        mouse_point.y = event->pos().y();
        QWidget::mouseMoveEvent(event);
    }

    void keyPressEvent(QKeyEvent *event) override {
        key_state    = LV_INDEV_STATE_PRESSED;
        auto keyCode = event->key();
        // 将 Qt 键码映射为 LVGL 中的虚拟键
        if(keyCode == Qt::Key_Up) {
            key_code = LV_KEY_UP;
        } else if(keyCode == Qt::Key_Down) {
            key_code = LV_KEY_DOWN;
        } else if(keyCode == Qt::Key_Left) {
            key_code = LV_KEY_LEFT;
        } else if(keyCode == Qt::Key_Right) {
            key_code = LV_KEY_RIGHT;
        } else if(keyCode == Qt::Key_Enter || keyCode == Qt::Key_Return) {
            key_code = LV_KEY_ENTER;
        } else if(keyCode == Qt::Key_Escape) {
            key_code = LV_KEY_ESC;
        } else if(keyCode == Qt::Key_Backspace) {
            key_code = LV_KEY_BACKSPACE;
        }
        qDebug() << event;
        QWidget::keyPressEvent(event);
    }

    void keyReleaseEvent(QKeyEvent *event) override {
        key_state = LV_INDEV_STATE_RELEASED;
        QWidget::keyReleaseEvent(event);
    }

    void paintEvent(QPaintEvent *event) override {
        QPainter painter(this);
        painter.drawImage(0, 0, image);
    }

private:
    inline static std::chrono::steady_clock::time_point system_start = std::chrono::steady_clock::now();

    int hor_res = 640;
    int ver_res = 480;
    QImage image;

    lv_display_t *display = nullptr;

    lv_indev_t *mouse_indev = nullptr;
    lv_indev_t *key_indev   = nullptr;

    lv_indev_state_t mouse_state = LV_INDEV_STATE_RELEASED;
    lv_point_t mouse_point       = {0, 0};

    lv_indev_state_t key_state = LV_INDEV_STATE_RELEASED;
    uint32_t key_code          = 0;
};
