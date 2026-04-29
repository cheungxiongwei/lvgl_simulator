docs
v1 采用 lvgl widget 方式绘制
v2 采用 lvlg 自定义绘制事件方式绘制
v3 采用 vector_graphic 矢量图像绘制

核心变换矩阵
```cpp
class Transform
{
public:
    Transform()  = default;

    ~Transform() = default;

    Transform& setTranslate(float tx, float ty) {
        this->tx = tx;
        this->ty = ty;
        markDirty();
        return *this;
    }

    Transform& setScale(float x, float y) {
        this->sx = x;
        this->sy = y;
        markDirty();
        return *this;
    }

    Transform& setRotate(float deg) {
        this->deg = deg;
        markDirty();
        return *this;
    }

    lv_matrix_t& matrix() {
        if (dirty) {
            updateLocalMatrix();
        }
        return localMatrix;
    }

    void markDirty() { dirty = true; }

    void updateLocalMatrix() {
        if (!dirty)
            return;

        lv_matrix_identity(&localMatrix);

        // T(tx, ty) * T(pivot) * R * S * T(-pivot)
        lv_matrix_translate(&localMatrix, tx, ty);
        lv_matrix_translate(&localMatrix, pivot_x, pivot_y);
        lv_matrix_rotate(&localMatrix, deg);
        lv_matrix_scale(&localMatrix, sx, sy);
        lv_matrix_translate(&localMatrix, -pivot_x, -pivot_y);

        dirty = false;
    }
private:
    float tx = 0, ty = 0;
    float sx = 1, sy = 1;
    float deg     = 0;
    float pivot_x = 0, pivot_y = 0;

    lv_matrix_t localMatrix;
    bool dirty = true;
};
```


| 类型        | 作用             |
| --------- | -------------- |
| Shape（图元） | 被画的东西（线、矩形、路径） |
| Tool（工具）  | 当前用户操作（画笔、填充）  |
| Style（样式） | Pen / Brush    |

命令系统（撤销 / 重做 / 操作记录）
```
struct Stroke {
    float width = 1.0f;
    lv_color_t color = lv_color_make(0, 255, 0);
};

struct Fill {
    lv_color_t color = lv_color_lighten(lv_color_black(), 50);
};

using Pen   = Stroke;
using Brush = Fill;

// 统一 Style（给 Shape 用）
struct Style {
    Stroke stroke;
    Fill fill;
};

// Shape 持有 Style（而不是 Pen/Brush）
struct Shape {
    Style style;
};

// 当前绘制状态
struct PaintContext {
    Stroke stroke;
    Fill fill;
};
```
