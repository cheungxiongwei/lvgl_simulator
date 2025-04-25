#pragma once

#include <QQuickItem>
#include <QSGSimpleTextureNode>
#include <QTimer>
#include <QApplication>

#include <chrono>

#include <lvgl.h>
#include <lvgl/src/display/lv_display_private.h>

/**
 * @class LVGLQML
 * @brief Qt Quick 与 LVGL 的集成类，允许在 QML 中使用 LVGL 图形库
 * 
 * 这个类将 LVGL 嵌入到 Qt Quick 场景中，处理渲染和输入事件转发
 */
class LVGLQML : public QQuickItem
{
    Q_OBJECT
    QML_NAMED_ELEMENT(Lvgl)  // 注册为 QML 类型，可在 QML 中使用 Lvgl 标签

public:
    /**
     * @brief 构造函数
     * @param parent 父 QQuickItem 对象
     */
    LVGLQML(QQuickItem *parent = nullptr)
      : QQuickItem(parent) {
        setFlag(ItemHasContents, true);           // 设置为有内容，这样 updatePaintNode 会被调用
        setAcceptedMouseButtons(Qt::AllButtons);  // 接受所有鼠标事件

        // 连接定时器信号到 LVGL 的时钟函数
        QObject::connect(&tickTimer, &QTimer::timeout, [&tickTimer = tickTimer]() {
            lv_tick_inc(tickTimer.interval());  // 更新 LVGL 的内部时钟
        });

        // 连接定时器信号到 LVGL 的任务处理函数
        QObject::connect(&taskTimer, &QTimer::timeout, [&]() {
            lv_timer_handler();  // 处理 LVGL 的任务队列
        });

        // 初始化 LVGL 驱动，设置屏幕大小为 640x480
        initLvglDriver(640, 480);  // TODO 根据 qml 设置的大小来调整，需要继承另外一个类，然后在里面初始化

        // 设置定时器间隔
        using namespace std::chrono_literals;
        tickTimer.start(5ms);   // 每 5 毫秒更新一次 LVGL 时钟
        taskTimer.start(40ms);  // 每 40 毫秒处理一次 LVGL 任务队列

        // 创建示例 UI 元素
        example();
    }

    /**
     * @brief 析构函数
     */
    ~LVGLQML() {
        // 释放输入设备
        if(mouse_indev) {
            lv_indev_delete(mouse_indev);
            mouse_indev = nullptr;
        }

        // 释放显示设备
        if(display) {
            lv_display_delete(display);
            display = nullptr;
        }

        // 清理 LVGL 资源
        // 注意：如果有多个 LVGL 实例，只应在最后一个实例销毁时调用
        lv_deinit();
    }

    /**
     * @brief 初始化 LVGL 驱动和显示
     * @param hor_res 水平分辨率
     * @param ver_res 垂直分辨率
     * 
     * 注意: 函数名有拼写错误，应为 "initLvglDriver" 而非 "intiLvglDriver"
     */
    void initLvglDriver(int hor_res, int ver_res) {
        // 参数验证
        if(hor_res <= 0 || ver_res <= 0) {
            qWarning() << "Invalid resolution:" << hor_res << "x" << ver_res;
            return;
        }

        // 初始化 LVGL 库
        lv_init();

        // 创建 LVGL 显示接口
        display = lv_display_create(hor_res, ver_res);

        // 设置用户数据，以便在回调中访问当前对象
        lv_display_set_user_data(display, this);

        // 启用抗锯齿
        lv_display_set_antialiasing(display, true);

        // 计算帧缓冲区大小并分配内存
        size_t sizeInBytes = display->hor_res * LV_COLOR_FORMAT_GET_SIZE(display->color_format) * display->ver_res;
        buffer             = std::make_unique<char[]>(sizeInBytes);

        // 创建 QImage 作为帧缓冲区
        framebuffer = QImage(display->hor_res, display->ver_res, QImage::Format_ARGB32);

        // 设置 LVGL 缓冲区
        lv_display_set_buffers(display, buffer.get(), NULL, sizeInBytes, LV_DISPLAY_RENDER_MODE_DIRECT);

        // 设置刷新回调函数，将 LVGL 渲染结果复制到 QImage
        lv_display_set_flush_cb(display, [](lv_display_t *display, const lv_area_t *area, uint8_t *pixel) {
            auto self = static_cast<LVGLQML *>(lv_display_get_user_data(display));
            Q_UNUSED(area);

            // 将像素数据复制到 QImage
            memcpy(self->framebuffer.bits(), pixel, display->hor_res * LV_COLOR_FORMAT_GET_SIZE(display->color_format) * display->ver_res);

            // 请求重绘
            self->update();

            // 通知 LVGL 刷新完成
            lv_disp_flush_ready(display);
        });

        // 创建鼠标输入设备
        mouse_indev = lv_indev_create();
        lv_indev_set_user_data(mouse_indev, this);
        lv_indev_set_type(mouse_indev, LV_INDEV_TYPE_POINTER);

        // 设置鼠标输入回调函数
        lv_indev_set_read_cb(mouse_indev, [](lv_indev_t *indev, lv_indev_data_t *data) {
            auto self = static_cast<LVGLQML *>(lv_indev_get_user_data(indev));

            // 获取全局鼠标位置
            QPoint globalPos = QCursor::pos();

            // 获取当前窗口
            QQuickWindow *window = self->window();
            if(!window) {
                data->state = LV_INDEV_STATE_RELEASED;  // 无窗口时鼠标状态为释放
                return;
            }

            // 将全局坐标转换为窗口坐标
            QPoint windowPos = window->mapFromGlobal(globalPos);

            // 检查鼠标是否在当前 Item 或其子项上
            QQuickItem *itemAtPoint = window->contentItem()->childAt(windowPos.x(), windowPos.y());
            bool isOnSelf           = (itemAtPoint == self || self->isAncestorOf(itemAtPoint));

            if(isOnSelf) {
                // 如果鼠标在当前 Item 上，转换为 Item 本地坐标
                QPointF localPos = self->mapFromScene(windowPos);
                data->point.x    = localPos.x();
                data->point.y    = localPos.y();

                // 根据鼠标左键状态设置按下/释放状态
                data->state = QGuiApplication::mouseButtons() & Qt::LeftButton ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
            } else {
                // 如果鼠标不在当前 Item 上，状态为释放
                data->state = LV_INDEV_STATE_RELEASED;
            }
        });

        // 检查创建是否成功
        if(!display) {
            qWarning() << "Failed to create LVGL display";
            return;
        }
    }

    /**
     * @brief 创建示例 UI 元素
     */
    void example() {
        // 创建一个按钮
        lv_obj_t *obj = lv_button_create(lv_screen_active());

        // 设置按钮大小和位置
        lv_obj_set_size(obj, 100, 50);
        lv_obj_set_pos(obj, 50, 50);

        // 添加点击事件处理
        lv_obj_add_event(obj, [](lv_event_t *e) { qDebug() << "clicked cheungxiongwei"; }, LV_EVENT_CLICKED, 0);

        // 创建标签并添加到按钮上
        lv_obj_t *label = lv_label_create(obj);
        lv_label_set_text(label, "Button");
        lv_obj_center(label);  // 将标签居中显示在按钮上
    }

protected:
    /**
     * @brief 更新场景图节点，将 QImage 渲染到场景中
     * @param oldNode 旧节点
     * @param updatePaintNodeData 更新数据
     * @return 更新后的节点
     */
    QSGNode *updatePaintNode(QSGNode *oldNode, QQuickItem::UpdatePaintNodeData *updatePaintNodeData) override {
        QSGSimpleTextureNode *node = static_cast<QSGSimpleTextureNode *>(oldNode);
        if(!node) {
            // 如果节点不存在，创建新节点
            node = new QSGSimpleTextureNode();
        }

        if(!framebuffer.isNull()) {
            // 从 QImage 创建纹理
            QSGTexture *texture = window()->createTextureFromImage(framebuffer);

            // 设置纹理并让节点拥有纹理的所有权，避免内存泄漏
            node->setTexture(texture);
            node->setOwnsTexture(true);

            // 设置节点矩形区域为当前 Item 的边界
            node->setRect(boundingRect());
        }

        return node;
    }

private:
    QTimer tickTimer;                // LVGL 时钟更新定时器
    QTimer taskTimer;                // LVGL 任务处理定时器
    std::unique_ptr<char[]> buffer;  // LVGL 绘图缓冲区
    QImage framebuffer;              // 用于转换为纹理的图像缓冲区

    lv_display_t *display {nullptr};
    lv_indev_t *mouse_indev {nullptr};
};
