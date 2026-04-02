# LVGraphics 模块说明

## 概述

`LVGraphics` 是基于 [LVGL](https://lvgl.io/) 图形库实现的一套轻量级图形编辑框架，提供可交互的图形元素管理能力，包括矩形图元的创建、选中、拖拽移动、拖拽旋转、拖拽缩放以及属性面板编辑等功能。

---

## 文件结构

| 文件               | 说明                                                 |
|------------------|----------------------------------------------------|
| `LVGraphics.h`   | 模块头文件，声明 `LVGraphics` 类及 `CreateLVGraphics()` 入口函数 |
| `LVGraphics.cpp` | 模块实现文件，包含所有核心类的完整实现                                |

---

## 核心类说明

### `LVGraphicsItem`

图形元素基类，封装了 LVGL 对象指针 (`lv_obj_t*`) 和变换状态 (`Transform`)。

- `rotate(float deg)`：累加旋转角度并应用变换
- `set_rotation(float deg)`：设置绝对旋转角度并应用变换
- `to_object()`：返回底层 LVGL 对象
- `transform()`：返回变换结构体引用

---

### `Transform`

描述图形元素的仿射变换状态：

| 字段    | 含义      | 默认值 |
|-------|---------|-----|
| `tx`  | X 轴平移   | 0.0 |
| `ty`  | Y 轴平移   | 0.0 |
| `sx`  | X 轴缩放   | 1.0 |
| `sy`  | Y 轴缩放   | 1.0 |
| `deg` | 旋转角度（度） | 0.0 |

`apply(lv_obj_t*)` 方法将变换以矩阵形式（T·S·R）应用到 LVGL 对象上。

---

### `OrientedBoundingBox` (OBB)

有向包围盒，用于计算旋转/缩放后图形元素的精确边界。

- 提供 `tl()` / `tr()` / `bl()` / `br()` 四个角点（经矩阵变换后的世界坐标）
- `aabb()`：从四角点计算轴对齐包围盒（AABB）
- `new_aabb(...)`：静态方法，从四个点直接计算 AABB

---

### `LVGraphicsRectItem`

继承自 `LVGraphicsItem`，表示一个矩形图元。

构造时接受位置 `(x, y)` 和尺寸 `(width, height)` 以及父对象，自动创建带灰色边框的 LVGL 对象。

---

### `LVGraphicsInteractGizmos`

交互控制手柄（Gizmo），负责选中图元后显示可拖拽的控制点，支持**拖拽旋转**和**拖拽缩放**。

**控制点布局：**

| 标识    | 位置   | 功能                 |
|-------|------|--------------------|
| `tl`  | 左上角  | 拖拽调整左上角（缩放）        |
| `tr`  | 右上角  | 拖拽调整右上角（缩放）        |
| `bl`  | 左下角  | 拖拽调整左下角（缩放）        |
| `br`  | 右下角  | 拖拽调整右下角（缩放）        |
| `tm`  | 顶部中点 | 拖拽调整顶边（缩放）         |
| `bm`  | 底部中点 | 拖拽调整底边（缩放）         |
| `lm`  | 左侧中点 | 拖拽调整左边（缩放）         |
| `rm`  | 右侧中点 | 拖拽调整右边（缩放）         |
| `rot` | 顶部上方 | **拖拽旋转**（连续角度跟随鼠标） |

**交互行为：**

- **拖拽移动**：拖拽 bbox 主体（非手柄区域）时，整体平移图元
- **拖拽旋转**：拖拽 `rot` 手柄时，根据鼠标相对于图元中心的角度差实时旋转图元（使用 `atan2` 计算）
- **拖拽缩放**：拖拽边/角手柄直接调整图元的位置和尺寸（通过 `solve_rect`），拖动哪边就仅改变那边，不修改 `sx`/`sy` 缩放因子

**主要方法：**

- `selected(LVGraphicsItem*)`：选中图元，更新 Gizmo 位置和变换
- `mouse_press/move/release`：处理鼠标拖拽事件
- `update_rotation(lv_point_t)`：根据鼠标位置计算并应用旋转角度
- `update_pos(lv_point_t)`：处理移动和边/角手柄拖拽缩放（直接修改位置和大小）
- `sync_bbox(lv_area_t)`：将 Gizmo 的包围盒同步回图元对象
- `contains(lv_obj_t*)`：判断某对象是否属于 Gizmo

---

### `LVGraphicsPropertyPanel`

属性面板，显示并允许编辑当前选中图元的属性。

**支持的属性字段：**

| 字段                 | 含义   |
|--------------------|------|
| `x` / `y`          | 图元位置 |
| `width` / `height` | 图元尺寸 |
| `tx` / `ty`        | 平移变换 |
| `sx` / `sy`        | 缩放变换 |
| `deg`              | 旋转角度 |

- `show_for_item(LVGraphicsItem*)`：绑定图元并显示面板
- `reset_properties()`：从图元读取当前状态刷新 UI
- `apply_transform()`：将 UI 输入写回图元变换并应用
- 输入框按下 Enter（`LV_EVENT_READY`）时自动触发属性更新

---

### `LVGraphicsScene`

场景容器，管理所有图元和交互组件。

- `add_item(LVGraphicsItem*)`：向场景添加图元
- `contains(lv_obj_t*)`：查找某 LVGL 对象对应的图元
- `mouse_press/move/release`：将鼠标事件分发给 Gizmo 和属性面板
- 持有 `LVGraphicsInteractGizmos*` 和 `LVGraphicsPropertyPanel*` 的引用

---

### `LVGraphicsView`

视图容器，占满父对象 100% 尺寸，负责监听 LVGL 鼠标事件并转发给场景。

- 监听 `LV_EVENT_PRESSED` / `LV_EVENT_PRESSING` / `LV_EVENT_RELEASED`
- 通过 `set_scene(LVGraphicsScene*)` 绑定场景

---

## 辅助工具

### `anchor_mask_t` 枚举

定义拖拽锚点方向掩码，用于 `solve_rect()` 计算拖拽后的新矩形区域：

| 值                     | 含义    |
|-----------------------|-------|
| `ANCHOR_LEFT`         | 拖拽左边  |
| `ANCHOR_RIGHT`        | 拖拽右边  |
| `ANCHOR_TOP`          | 拖拽上边  |
| `ANCHOR_BOTTOM`       | 拖拽下边  |
| `ANCHOR_TOP_LEFT`     | 拖拽左上角 |
| `ANCHOR_TOP_RIGHT`    | 拖拽右上角 |
| `ANCHOR_BOTTOM_LEFT`  | 拖拽左下角 |
| `ANCHOR_BOTTOM_RIGHT` | 拖拽右下角 |
| `ANCHOR_CENTER`       | 整体移动  |
| `ANCHOR_ROTATE`       | 拖拽旋转  |

### `solve_rect(lv_area_t, int anchor_mask, int dx, int dy)`

根据锚点掩码和鼠标偏移量计算新的矩形区域，并保证最小尺寸为 1×1。

---

## 入口函数

```cpp
void CreateLVGraphics();
```

创建完整的图形编辑场景，包括：

1. 创建 `LVGraphicsView`（全屏视图）
2. 创建 `LVGraphicsScene`（场景）
3. 创建 `LVGraphicsInteractGizmos`（交互控制手柄）
4. 创建 `LVGraphicsPropertyPanel`（属性面板，右侧居中对齐）
5. 添加 4 个示例矩形图元
6. 对第一个图元执行 45° 旋转

---

## 依赖

- [LVGL](https://lvgl.io/) v9+（需支持 `lv_matrix_t`、`lv_obj_set_transform` 等接口）
- C++23 标准库（使用了 `<print>`）
- C 标准数学库 `<cmath>`（用于 `atan2f` 旋转角度计算）
