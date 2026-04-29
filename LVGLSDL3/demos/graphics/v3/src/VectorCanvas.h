#pragma once
#include <cmath>
#include <functional>
#include <map>
#include <optional>
#include <print>
#include <stack>
#include <algorithm>
#include <ranges>
#include <tuple>
#include <utility>
#include <concepts>
#include <variant>
#include <vector>
#include "lvgl.h"

class Transform
{
public:
    Transform() { lv_matrix_identity(&m_matrix); };

    explicit Transform(lv_matrix_t const& mat) : m_matrix(mat) {}

    ~Transform() = default;

    Transform& reset() {
        lv_matrix_identity(&m_matrix);
        return *this;
    }

    Transform& translate(float tx, float ty) {
        lv_matrix_translate(&m_matrix, tx, ty);
        return *this;
    }

    Transform& scale(float sx, float sy) {
        lv_matrix_scale(&m_matrix, sx, sy);
        return *this;
    }

    Transform& scale(float s) { return scale(s, s); }

    Transform& scaleAround(float sx, float sy, float pivot_x, float pivot_y) {
        Transform local;
        local.translate(-pivot_x, -pivot_y).scale(sx, sy).translate(pivot_x, pivot_y);
        return multiply(local);
    }

    Transform& rotate(float rotation) {
        lv_matrix_rotate(&m_matrix, rotation);
        return *this;
    }

    Transform& rotateAround(float rotation, float pivot_x, float pivot_y) {
        Transform local;
        local.translate(-pivot_x, -pivot_y).rotate(rotation).translate(pivot_x, pivot_y);
        return multiply(local);
    }

    Transform& multiply(Transform const& transform) {
        lv_matrix_multiply(&m_matrix, &transform.m_matrix);
        return *this;
    }

    Transform& multiply(lv_matrix_t const& mat) {
        lv_matrix_multiply(&m_matrix, &mat);
        return *this;
    }

    bool invert() {
        lv_matrix_t m;
        lv_matrix_identity(&m);
        bool ret = lv_matrix_inverse(&m, &m_matrix);
        if (ret) {
            m_matrix = m;
        }
        return ret;
    }

    bool invertFrom(Transform const& transform) { return lv_matrix_inverse(&m_matrix, &transform.m_matrix); }

    void transpose(Transform& out) const { lv_matrix_transpose(&m_matrix, &out.m_matrix); }

    lv_point_precise_t transformPoint(lv_point_precise_t const& pt) const;

    lv_matrix_t& raw() noexcept { return m_matrix; }
private:
    lv_matrix_t m_matrix;
};

struct TransformState {
    float tx{0.0f};
    float ty{0.0f};
    float sx{1.0f};
    float sy{1.0f};
    float rotation{0.0f};
    float pivot_x{0.0f};
    float pivot_y{0.0f};
};

/*# example
Defer defer([&]() { restore(); });
*/
template <typename F>
requires std::invocable<F>
class Defer
{
public:
    explicit Defer(F&& f) : fn(std::forward<F>(f)), active(true) {}

    Defer(Defer const&)            = delete;
    Defer& operator=(Defer const&) = delete;

    Defer(Defer&& other) noexcept : fn(std::move(other.fn)), active(other.active) { other.active = false; }

    ~Defer() {
        if (active) {
            fn();
        }
    }
private:
    F fn;
    bool active;
};

struct Stroke {
    float width        = 1.0f;
    lv_color32_t color = lv_color_to_32(lv_color_hex(0x00ff00), 0xff);
    lv_opa_t opa       = LV_OPA_COVER;
    lv_vector_stroke_cap_t cap;
    lv_vector_stroke_join_t join;
};

struct Fill {
    lv_color32_t color = lv_color_to_32(lv_color_lighten(lv_color_black(), 50), 0xff);
    lv_opa_t opa       = LV_OPA_COVER;
};

struct Style {
    Stroke stroke;
    Fill fill;
};

enum class GeometryType {
    Point,
    Line,
    Rect,
    Circle,
    Polygon,
    Path,
    Bezier,
};

struct BezierAnchorPoint {
    lv_fpoint_t p;
    std::optional<lv_fpoint_t> in;
    std::optional<lv_fpoint_t> out;
};

struct GeometryData {
    GeometryType type;

    std::vector<lv_fpoint_t> points;
    std::vector<BezierAnchorPoint> beziers;
};

enum class ShapeType {
    Vector,
    Text,
    Image
};

struct TextData {
    std::string text;

    float font_size{14.0f};
    std::string font_family;
};

struct ImageData {
    std::string uri;

    float width{0};
    float height{0};
};

using ShapeContent = std::variant<GeometryData, TextData, ImageData>;

/*#
采用 std::variant 组织多态数据结构，并配合 std::visit 进行类型匹配与分发，使不同数据类型能够调用各自的绘制逻辑。

定义常用的属性，id、visible、locked、editable
*/
struct Shape {
    uint64_t id = 0;

    std::string name;

    ShapeType type{ShapeType::Vector};
    ShapeContent content;
    Style style;
    TransformState transform;

    bool visible{true};
    bool locked{false};
    bool editor{false};

    void reset() {
        name      = "";
        content   = GeometryData{};
        style     = Style();
        transform = TransformState();
        visible   = true;
        locked    = false;
    }
};

class VectorPath
{
public:
    explicit VectorPath(lv_vector_path_quality_t quality = LV_VECTOR_PATH_QUALITY_MEDIUM) { path = lv_vector_path_create(quality); }

    explicit VectorPath(lv_vector_path_t const* path, lv_vector_path_quality_t quality = LV_VECTOR_PATH_QUALITY_MEDIUM) {
        this->path = lv_vector_path_create(quality);
        lv_vector_path_copy(this->path, path);
    }

    ~VectorPath() { lv_vector_path_delete(path); }

    void clear() { lv_vector_path_clear(path); }

    void move_to(lv_fpoint_t const& point) { lv_vector_path_move_to(path, &point); }

    void line_to(lv_fpoint_t const& point) { lv_vector_path_line_to(path, &point); }

    void quad_to(lv_fpoint_t const& control_point, lv_fpoint_t const& end_point) { lv_vector_path_quad_to(path, &control_point, &end_point); }

    void cubic_to(lv_fpoint_t const& control_point1, lv_fpoint_t const& control_point2, lv_fpoint_t const& end_point) {
        lv_vector_path_cubic_to(path, &control_point1, &control_point2, &end_point);
    }

    /**
     * @brief 从当前路径点到指定终点，添加一段椭圆弧
     *
     * 该函数是对 lv_vector_path_arc_to 的简单封装，用于在路径中绘制一段
     * 椭圆弧（类似 SVG 的 A 指令）。
     *
     * 椭圆弧由起点（当前路径点）和终点（end_point）确定，中间路径由以下参数控制：
     *
     * @param end_point    弧线的终点坐标（起点为当前路径最后一个点）
     * @param radius_x     椭圆在 x 方向的半径
     * @param radius_y     椭圆在 y 方向的半径
     * @param rotate_angle 椭圆相对于坐标系的旋转角度（单位通常为度）
     *
     * @param large_arc    是否选择大弧：
     *                     - false：选择较短路径（小弧）
     *                     - true ：选择较长路径（大弧）
     *
     * @param clockwise    弧线方向：
     *                     - true ：顺时针方向
     *                     - false：逆时针方向
     *
     * @note 在相同起点和终点下，共存在 4 种可能的弧线组合：
     *       (large_arc × clockwise)，该函数用于选择其中一种。
     */
    void arc_to(lv_fpoint_t const& end_point, float radius_x, float radius_y, float rotate_angle, bool large_arc, bool clockwise) {
        lv_vector_path_arc_to(path, radius_x, radius_y, rotate_angle, large_arc, clockwise, &end_point);
    }

    void round_rect(float x, float y, float width, float height, float radius) { lv_vector_path_append_rectangle(path, x, y, width, height, radius, radius); }

    void circle(lv_fpoint_t const& center_point, float radius) { lv_vector_path_append_circle(path, &center_point, radius, radius); }

    void circle(lv_fpoint_t const& center_point, float radius_x, float radius_y) { lv_vector_path_append_circle(path, &center_point, radius_x, radius_y); }

    void arc(lv_fpoint_t const& center_point, float radius) { lv_vector_path_append_circle(path, &center_point, radius, radius); }

    void arc(lv_fpoint_t const& center_point, float radius_x, float radius_y) { lv_vector_path_append_circle(path, &center_point, radius_x, radius_y); }

    void add_path(VectorPath const& subpath) { lv_vector_path_append_path(path, subpath.path); }

    void add_path(lv_vector_path_t const* subpath) { lv_vector_path_append_path(path, subpath); }

    void close() { lv_vector_path_close(path); }

    lv_area_t bounding_box() {
        lv_area_t bounding_box;
        lv_vector_path_get_bounding(path, &bounding_box);
        return bounding_box;
    }
private:
    friend class VectorPainter;
    lv_vector_path_t* path{nullptr};
};

/*#
 * 简洁、流畅、偏 API 体验
 */
class VectorPainter
{
public:
    explicit VectorPainter(lv_layer_t* layer) { context = lv_draw_vector_dsc_create(layer); }

    ~VectorPainter() {
        lv_draw_vector(context);
        lv_draw_vector_dsc_delete(context);
    }

    void draw_point(float x, float y, float r = 3) { draw_point({x, y}, r); }

    void draw_point(lv_fpoint_t const& p, float r = 3) {
        draft.clear();
        draft.circle(p, r);

        save();
        set_stroke_color(std::nullopt);
        stroke();
        fill();
        add_path(draft);
        restore();
    }

    void draw_line(lv_fpoint_t const& p1, lv_fpoint_t const& p2) {
        draft.clear();
        draft.move_to(p1);
        draft.line_to(p2);
        draft.close();

        save();
        set_fill_color(std::nullopt);
        stroke();
        fill();
        add_path(draft);
        restore();
    }

    void draw_dash_line(lv_fpoint_t const& p1, lv_fpoint_t const& p2) {
        static float dash_pattern[] = {10.0, 10.0, 2.0, 10.0};

        draft.clear();
        draft.move_to(p1);
        draft.line_to(p2);
        draft.close();

        save();
        set_fill_color(std::nullopt);
        lv_draw_vector_dsc_set_stroke_dash(context, dash_pattern, sizeof(dash_pattern) / sizeof(float));
        add_path(draft);
        lv_draw_vector_dsc_set_stroke_dash(context, nullptr, 0);
        restore();
    }

    void draw_arrow(std::vector<lv_fpoint_t> const& lines, float arrow_size = 20.0f) {
        if (lines.size() < 2)
            return;

        auto p0   = lines[lines.size() - 2];
        auto p1   = lines[lines.size() - 1];

        float dx  = p1.x - p0.x;
        float dy  = p1.y - p0.y;

        float len = std::sqrt(dx * dx + dy * dy);
        if (len == 0)
            return;

        dx /= len;
        dy /= len;

        float angle       = 0.5f;

        auto rot          = [&](float x, float y, float a) { return lv_fpoint_t{x * std::cos(a) - y * std::sin(a), x * std::sin(a) + y * std::cos(a)}; };

        auto L            = rot(dx, dy, angle);
        auto R            = rot(dx, dy, -angle);

        lv_fpoint_t tip   = p1;
        lv_fpoint_t left  = {p1.x - L.x * arrow_size, p1.y - L.y * arrow_size};
        lv_fpoint_t right = {p1.x - R.x * arrow_size, p1.y - R.y * arrow_size};

        draw_polygon({tip, left, right}, true);
    }

    void draw_polyline(std::vector<lv_fpoint_t> const& lines, bool close = false) {
        if (lines.size() < 2)
            return;
        draft.clear();
        draft.move_to(lines[0]);
        for (size_t i = 1; i < lines.size(); ++i) {
            draft.line_to(lines[i]);
        }
        if (close)
            draft.close();

        save();
        set_fill_color(std::nullopt);
        stroke();
        fill();
        add_path(draft);
        restore();
    }

    void draw_polygon(std::vector<lv_fpoint_t> const& lines, bool close = false) {
        if (lines.size() < 2)
            return;
        draft.clear();
        draft.move_to(lines[0]);
        for (size_t i = 1; i < lines.size(); ++i) {
            draft.line_to(lines[i]);
        }
        if (close)
            draft.close();

        save();
        set_stroke_color(std::nullopt);
        stroke();
        fill();
        add_path(draft);
        restore();
    }

    void draw_curve(lv_fpoint_t const& p_start, lv_fpoint_t const& p1, lv_fpoint_t const& p2, lv_fpoint_t const& p_end) {
        draw_point(p_start);

        draft.clear();
        draft.move_to(p1);
        draft.cubic_to(p1, p2, p_end);

        save();
        set_fill_color(std::nullopt);
        stroke();
        add_path(draft);
        restore();
    }

    void add_path(VectorPath const& path) {
        save();
        stroke();
        fill();
        lv_draw_vector_dsc_add_path(context, path.path);
        restore();
    }

    void add_path(VectorPath const& path, lv_matrix_t const* matrix) {
        lv_matrix_transform_path(matrix, path.path);
        lv_draw_vector_dsc_add_path(context, path.path);
    }

    /* can not work,use layer lv_draw_image */
    void add_image(lv_draw_image_dsc_t const* img_dsc) {
        lv_vector_path_t* path = lv_vector_path_create(LV_VECTOR_PATH_QUALITY_MEDIUM);

        lv_fpoint_t start      = {0, 0};
        lv_fpoint_t width      = {img_dsc->header.w, 0};
        lv_fpoint_t size       = {img_dsc->header.w, img_dsc->header.h};
        lv_fpoint_t height     = {0, img_dsc->header.h};

        lv_vector_path_move_to(path, &start);
        lv_vector_path_line_to(path, &width);
        lv_vector_path_line_to(path, &size);
        lv_vector_path_line_to(path, &height);
        lv_vector_path_close(path);

        lv_draw_vector_dsc_set_fill_image(context, img_dsc);

        lv_draw_vector_dsc_add_path(context, path);
        lv_draw_vector_dsc_set_fill_color(context, lv_color_make(0, 255, 0));
    }

    void add_text(std::string const& text) {
        //
        // lv_draw_label
    }

    void set_stroke_width(float width) { style.stroke.width = width; }

    void set_stroke_color(std::optional<lv_color_t> const& color) {
        if (color) {
            style.stroke.color = lv_color_to_32(*color, 0xff);
            style.stroke.opa   = LV_OPA_COVER;
        } else {
            style.stroke.opa = LV_OPA_TRANSP;
        }
    }

    void set_fill_color(std::optional<lv_color_t> const& color) {
        if (color) {
            style.fill.color = lv_color_to_32(*color, 0xff);
            style.fill.opa   = LV_OPA_COVER;
        } else {
            style.fill.opa = LV_OPA_TRANSP;
        }
    }

    void set_style(Style const& style) { this->style = style; }

    void save() { style_state.emplace(style); }

    void restore() {
        if (!style_state.empty()) {
            style = style_state.top();
            stroke();
            fill();
            style_state.pop();
        }
    }

    void clear(lv_area_t const& rect, lv_color_t const& color = lv_color_lighten(lv_color_black(), 50)) {
        save();
        lv_draw_vector_dsc_set_fill_color(context, color);
        lv_draw_vector_dsc_set_fill_opa(context, LV_OPA_COVER);
        lv_draw_vector_dsc_clear_area(context, &rect);
        restore();
    }

    void translate(float tx, float ty) { lv_draw_vector_dsc_translate(context, tx, ty); }

    void scale(float s) { lv_draw_vector_dsc_scale(context, s, s); }

    void scale(float s, float pivot_x, float pivot_y) {
        translate(-pivot_x, -pivot_y);
        scale(s);
        translate(pivot_x, pivot_y);
    }

    void scale(float sx, float sy) { lv_draw_vector_dsc_scale(context, sx, sy); }

    void rotate(float angle) { lv_draw_vector_dsc_rotate(context, angle); }

    void rotate(float angle, float pivot_x, float pivot_y) {
        translate(-pivot_x, -pivot_y);
        rotate(angle);
        translate(pivot_x, pivot_y);
    }

    void set_transform(lv_matrix_t const* matrix) { lv_draw_vector_dsc_set_transform(context, matrix); }
private:
    void stroke() {
        lv_draw_vector_dsc_set_stroke_width(context, style.stroke.width);
        lv_draw_vector_dsc_set_stroke_color32(context, style.stroke.color);
        lv_draw_vector_dsc_set_stroke_opa(context, style.stroke.opa);
    }

    void fill() {
        lv_draw_vector_dsc_set_fill_color32(context, style.fill.color);
        lv_draw_vector_dsc_set_fill_opa(context, style.fill.opa);
    }
private:
    lv_draw_vector_dsc_t* context;
    Style style;
    std::stack<Style> style_state;
    VectorPath draft;
};

struct Layer {
    uint64_t id;
    std::vector<Shape> shapes;
};

class IconButton
{
public:
    enum class Type : int {
        Select,
        Brush,
        VectorDraw,
        Draw,
        Start,
        ImageAdd,
        Image,
        Text,
        D3,
        Erase,
        Delete,
        Visibility,
        VisibilityOff,
        Count
    };

    struct icon_t {
        char const* name;
        char const* svg;
    };

    IconButton(Type type, lv_obj_t* parent) : m_type(type) {
        m_obj = lv_obj_create(parent);
        lv_obj_remove_style_all(m_obj);
        lv_obj_add_flag(m_obj, LV_OBJ_FLAG_CHECKABLE);
        lv_obj_set_scroll_dir(m_obj, LV_DIR_NONE);
        lv_obj_set_size(m_obj, 32, 32);
        lv_obj_set_style_radius(m_obj, LV_RADIUS_CIRCLE, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(m_obj, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_border_opa(m_obj, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_bg_color(m_obj, lv_color_make(248, 248, 248), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(m_obj, lv_color_make(225, 225, 225), LV_PART_MAIN | LV_STATE_HOVERED);
        lv_obj_set_style_bg_color(m_obj, lv_color_make(59, 99, 251), LV_PART_MAIN | LV_STATE_CHECKED);
        lv_obj_set_style_bg_color(m_obj, lv_color_make(59, 99, 251), LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_bg_color(m_obj, lv_color_make(233, 233, 233), LV_PART_MAIN | LV_STATE_DISABLED);

        lv_obj_set_style_pad_all(m_obj, 6, LV_PART_MAIN);
        lv_obj_set_style_margin_all(m_obj, 6, LV_PART_MAIN);

        lv_obj_set_style_shadow_width(m_obj, 8, 0);
        lv_obj_set_style_shadow_color(m_obj, lv_color_hex(0x000000), LV_PART_MAIN);
        lv_obj_set_style_shadow_opa(m_obj, LV_OPA_10, 0);

        lv_obj_set_style_translate_y(m_obj, 1, LV_STATE_PRESSED);

        lv_obj_add_event_cb(m_obj, [](auto e) { static_cast<IconButton*>(lv_event_get_user_data(e))->paint(e); }, LV_EVENT_DRAW_MAIN, this);
        lv_obj_add_event_cb(
            m_obj,
            [](auto e)
            {
                auto self     = static_cast<IconButton*>(lv_event_get_user_data(e));
                lv_obj_t* obj = self->m_obj;
                if (lv_obj_has_state(obj, LV_STATE_CHECKED)) {
                    lv_obj_remove_style(obj, NULL, LV_PART_MAIN | LV_STATE_HOVERED);
                } else {
                    lv_obj_set_style_bg_color(obj, lv_color_make(225, 225, 225), LV_PART_MAIN | LV_STATE_HOVERED);
                }
            },
            LV_EVENT_STATE_CHANGED,
            this);

        lv_obj_add_event_cb(
            m_obj,
            [](auto e)
            {
                auto obj = lv_event_get_target_obj(e);

                if (!lv_obj_has_flag(obj, LV_OBJ_FLAG_CHECKABLE))
                    return;

                auto parent = lv_obj_get_parent(obj);
                if (!parent)
                    return;

                uint32_t cnt = lv_obj_get_child_count(parent);
                for (uint32_t i = 0; i < cnt; ++i) {
                    lv_obj_t* child = lv_obj_get_child(parent, i);
                    if (lv_obj_has_flag(child, LV_OBJ_FLAG_CHECKABLE))
                        lv_obj_remove_state(child, LV_STATE_CHECKED);
                }

                lv_obj_add_state(obj, LV_STATE_CHECKED);
            },
            LV_EVENT_CLICKED,
            nullptr);

        // clang-format off
            // https://react-spectrum.adobe.com/
            static icon_t const icons[(int)Type::Count] =
                {
                    { .name = "Select",         .svg = R"r(<svg xmlns="http://www.w3.org/2000/svg" width="20" height="20" fill="none" viewBox="0 0 20 20"><path fill="black" d="M6.281 18.547c-.292 0-.589-.058-.876-.177-.856-.355-1.387-1.151-1.387-2.078l-.004-12.47c0-.928.532-1.725 1.39-2.08.854-.354 1.796-.168 2.452.489l8.892 8.914c.655.655.84 1.594.486 2.45-.356.855-1.152 1.387-2.079 1.387h-4.078c-.197 0-.39.08-.53.22l-2.691 2.683c-.436.434-.997.662-1.575.662M6.277 3.068c-.137 0-.25.039-.3.06-.109.045-.463.228-.463.693l.004 12.471c0 .465.354.648.462.692.108.045.487.167.817-.162l2.691-2.682c.426-.424.99-.658 1.589-.658h4.078c.465 0 .648-.354.693-.462s.166-.488-.161-.816L6.795 3.29c-.174-.174-.362-.222-.518-.222"></path></svg>)r"                                                                                                                                                                                                                                                                                                                        },
                    { .name = "Brush",          .svg = R"r(<svg xmlns="http://www.w3.org/2000/svg" width="20" height="20" fill="none" viewBox="0 0 20 20"><path fill="black" d="M18.074 1.6c-.528-.44-1.192-.654-1.883-.587-.685.061-1.306.386-1.745.912-.036.043-5.03 6.019-6.693 8.012-.344.016-.693.067-1.042.164-1.973.546-2.687 2.232-3.315 3.718-.501 1.184-.975 2.302-2.003 2.859-.287.154-.44.476-.38.797.06.319.318.564.641.606.936.12 1.977.208 3.024.208 2.16 0 4.351-.372 5.72-1.594.925-.825 1.385-1.937 1.366-3.306-.001-.063-.02-.122-.024-.184l1.206-1.443c2.103-2.517 5.381-6.443 5.455-6.53.912-1.09.765-2.72-.327-3.631M9.4 15.576c-1.052.94-3.109 1.345-5.865 1.18.554-.725.91-1.57 1.241-2.35.57-1.345 1.06-2.506 2.335-2.86.875-.24 1.732-.112 2.354.353.5.375.79.926.799 1.512.012.927-.27 1.636-.864 2.165m7.853-11.308c-.043.05-3.345 4.002-5.458 6.533l-.634.76c-.218-.32-.472-.619-.797-.862-.273-.204-.574-.36-.89-.484 2.01-2.407 6.059-7.251 6.122-7.325.183-.221.443-.357.73-.383.292-.023.566.062.787.247.456.38.518 1.061.14 1.514"></path></svg>)r"    },
                    { .name = "VectorDraw",     .svg = R"r(<svg xmlns="http://www.w3.org/2000/svg" width="20" height="20" fill="none" viewBox="0 0 20 20"><path fill="black" d="m17.678 5.251-2.96-2.959c-.849-.85-2.332-.85-3.182 0L10.29 3.539q-.009.012-.016.024L5.79 6.01c-.574.314-1.02.823-1.253 1.435L1.22 16.162c-.278.733-.1 1.563.454 2.118.382.382.89.584 1.408.584.251 0 .503-.047.745-.143L13.075 15c.623-.25 1.133-.719 1.435-1.322l2.078-4.157 1.09-1.09c.877-.875.877-2.303 0-3.181m-4.51 7.757c-.137.274-.368.487-.652.6L4.08 17.001l4.262-4.262c.053.008.101.032.157.032.69 0 1.25-.56 1.25-1.25s-.56-1.25-1.25-1.25-1.25.56-1.25 1.25c0 .056.025.104.032.158l-4.44 4.44 3.097-8.14c.106-.279.309-.511.57-.654l4.357-2.377 4.222 4.222zm3.45-5.636-.604.603-4.02-4.02.603-.602c.283-.284.777-.284 1.06 0l2.96 2.959c.292.292.292.768 0 1.06"></path></svg>)r"                                                                                                                                                                                                          },
                    { .name = "Draw",           .svg = R"r(<svg xmlns="http://www.w3.org/2000/svg" width="20" height="20" fill="none" viewBox="0 0 20 20"><path fill="black" d="M17.53 1.999c-1.27-1.042-3.225-.9-4.449.324L3.105 12.3c-.319.317-.557.716-.689 1.15l-1.384 4.584c-.08.265-.008.552.188.747.142.143.334.22.53.22q.11 0 .217-.032l4.585-1.384c.433-.132.832-.37 1.148-.688L17.777 6.818c.649-.647.994-1.544.949-2.46S18.244 2.583 17.53 2M7.468 15.006l-2.48-2.47 6.858-6.857 2.475 2.475zm-4.596 2.122.98-3.244c.026-.09.066-.173.11-.252l2.413 2.402q-.122.072-.258.114zm13.845-11.37L15.38 7.093 12.907 4.62l1.235-1.235c.386-.387.896-.586 1.39-.586.38 0 .751.118 1.049.361.392.321.621.774.647 1.274.024.493-.163.976-.511 1.325"></path></svg>)r"},
                    { .name = "Start",          .svg = R"r(<svg xmlns="http://www.w3.org/2000/svg" width="20" height="20" fill="none" viewBox="0 0 20 20"><path fill="black" d="M5.506 18.63c-.36 0-.719-.112-1.027-.337-.594-.431-.854-1.168-.66-1.876l.977-3.577c.079-.29-.02-.596-.254-.783l-2.893-2.32c-.573-.459-.796-1.207-.57-1.906.228-.698.848-1.172 1.582-1.206l3.703-.176c.3-.014.56-.204.665-.483l1.313-3.469c.26-.687.902-1.131 1.637-1.131s1.376.444 1.636 1.13v.001l1.31 3.468c.107.28.369.47.667.484l3.704.176c.733.034 1.354.508 1.582 1.207.226.698.003 1.447-.57 1.906l-2.894 2.32c-.232.186-.333.493-.254.782l.977 3.574c.193.71-.067 1.448-.662 1.879-.598.434-1.38.447-1.994.041l-3.07-2.036c-.252-.166-.576-.166-.827-.002l-3.117 2.045c-.295.193-.628.29-.961.29M9.979 2.866c-.08 0-.184.028-.235.162L8.433 6.496c-.317.839-1.101 1.409-1.998 1.451l-3.704.176c-.142.006-.2.097-.225.172s-.03.183.08.273l2.893 2.319c.702.56 1.001 1.483.764 2.349l-.977 3.576c-.038.137.03.222.094.268.063.045.162.086.285.007l3.117-2.045c.753-.493 1.725-.492 2.476.005l3.072 2.037c.12.08.221.04.285-.006.064-.047.133-.13.094-.268l-.976-3.575c-.236-.866.063-1.787.764-2.348l2.893-2.32c.111-.088.106-.197.081-.272s-.083-.166-.225-.172l-3.705-.176c-.895-.042-1.68-.612-1.998-1.45l-1.31-3.47c-.05-.133-.155-.161-.234-.161"></path></svg>)r"},
                    { .name = "ImageAdd",       .svg = R"r(<svg xmlns="http://www.w3.org/2000/svg" width="20" height="20" fill="none" viewBox="0 0 20 20"><path fill="black" d="M14.5 7.521c0 .829-.672 1.5-1.5 1.5s-1.5-.671-1.5-1.5.672-1.5 1.5-1.5 1.5.672 1.5 1.5M15 10.5c-2.485 0-4.5 2.015-4.5 4.5s2.015 4.5 4.5 4.5 4.5-2.015 4.5-4.5-2.015-4.5-4.5-4.5m2.5 5.125h-1.875V17.5c0 .345-.28.625-.625.625s-.625-.28-.625-.625v-1.875H12.5c-.345 0-.625-.28-.625-.625s.28-.625.625-.625h1.875V12.5c0-.345.28-.625.625-.625s.625.28.625.625v1.875H17.5c.345 0 .625.28.625.625s-.28.625-.625.625"></path><path fill="black" d="M16.75 3H3.25C2.01 3 1 4.01 1 5.25v9.5C1 15.99 2.01 17 3.25 17h5.09c.414 0 .75-.336.75-.75s-.336-.75-.75-.75H3.25c-.413 0-.75-.337-.75-.75v-1.169l2.97-2.969c.293-.293.767-.293 1.06 0l1.915 1.915c.293.293.768.293 1.06 0s.294-.767 0-1.06L7.592 9.552c-.877-.877-2.305-.877-3.182 0L2.5 11.46V5.25c0-.413.337-.75.75-.75h13.5c.413 0 .75.337.75.75v3.847c0 .414.336.75.75.75s.75-.336.75-.75V5.25C19 4.01 17.99 3 16.75 3"></path></svg>)r"},
                    { .name = "Image",          .svg = R"r(<svg xmlns="http://www.w3.org/2000/svg" width="20" height="20" fill="none" viewBox="0 0 20 20"><path fill="black" d="M14.5 7.521c0 .829-.672 1.5-1.5 1.5s-1.5-.671-1.5-1.5c0-.828.672-1.5 1.5-1.5s1.5.672 1.5 1.5"></path><path fill="black" d="M16.75 3H3.25C2.01 3 1 4.01 1 5.25v9.5C1 15.99 2.01 17 3.25 17h13.5c1.24 0 2.25-1.01 2.25-2.25v-9.5C19 4.01 17.99 3 16.75 3M3.25 4.5h13.5c.413 0 .75.337.75.75v8.21l-1.91-1.908c-.876-.877-2.304-.877-3.18 0l-1.232 1.231c-.1.098-.257.097-.355.001L7.591 9.552c-.85-.85-2.332-.85-3.182 0L2.5 11.46V5.25c0-.413.337-.75.75-.75m0 11c-.413 0-.75-.337-.75-.75v-1.168l2.97-2.97c.293-.293.767-.293 1.06 0l3.234 3.234c.681.68 1.792.68 2.473-.001l1.233-1.233c.293-.293.767-.293 1.06 0l2.701 2.701c-.131.112-.296.187-.481.187z"></path></svg>)r"},
                    { .name = "Text",           .svg = R"r(<svg xmlns="http://www.w3.org/2000/svg" width="20" height="20" fill="none" viewBox="0 0 20 20"><path fill="black" d="M14.59 2H5.41C4.17 2 3.16 3.01 3.16 4.25v1.11c0 .414.336.75.75.75s.75-.336.75-.75V4.25c0-.413.337-.75.75-.75h3.84v13H7.68c-.414 0-.75.336-.75.75s.336.75.75.75h4.64c.414 0 .75-.336.75-.75s-.336-.75-.75-.75h-1.57v-13h3.84c.413 0 .75.337.75.75v1.11c0 .414.336.75.75.75s.75-.336.75-.75V4.25c0-1.24-1.01-2.25-2.25-2.25"></path></svg>)r"},
                    { .name = "3d",             .svg = R"r(<svg xmlns="http://www.w3.org/2000/svg" width="20" height="20" fill="none" viewBox="0 0 20 20"><path fill="black" d="M16.876 4.84h-.001l-5.749-3.319c-.696-.402-1.559-.4-2.251 0l-5.751 3.32C2.431 5.242 2 5.989 2 6.79v6.64c0 .801.432 1.548 1.125 1.948l5.749 3.32C9.222 18.9 9.611 19 10 19c.39 0 .778-.1 1.125-.3l5.75-3.321c.693-.4 1.125-1.147 1.125-1.948V6.79c0-.8-.43-1.548-1.124-1.95m-7.25-2.02c.115-.067.244-.1.374-.1.129 0 .259.033.375.1l5.285 3.052L10 8.994l-5.664-3.12zM3.875 14.08c-.231-.135-.375-.384-.375-.65V7.126l5.75 3.168v6.889zm12.25 0-5.375 3.102v-6.889l5.75-3.172v6.309c0 .266-.144.515-.375.65"></path></svg>)r"},
                    { .name = "Erase",          .svg = R"r(<svg xmlns="http://www.w3.org/2000/svg" width="20" height="20" fill="none" viewBox="0 0 20 20"><path fill="black" d="m18.184 5.868-3.688-3.687c-.85-.85-2.333-.85-3.182 0l-9.498 9.497c-.876.876-.877 2.301-.002 3.178l2.482 2.492c.42.42 1.001.662 1.594.662h2.405c.593 0 1.172-.24 1.591-.66l8.299-8.3c.876-.878.876-2.305-.001-3.182M8.826 16.29c-.14.14-.333.22-.53.22H5.89c-.197 0-.391-.08-.531-.22l-2.483-2.492c-.291-.293-.291-.768.001-1.06l2.636-2.636 4.59 4.59c.057.057.125.093.193.128zm8.298-8.3-5.833 5.834c-.035-.068-.071-.136-.128-.193l-4.59-4.59 5.802-5.8c.282-.283.777-.283 1.06 0l3.688 3.688c.293.292.293.768 0 1.06M17.937 18.021H17.5c-.414 0-.75-.335-.75-.75s.336-.75.75-.75h.437c.414 0 .75.336.75.75s-.336.75-.75.75M15.006 18.021H12.5c-.414 0-.75-.335-.75-.75s.336-.75.75-.75h2.506c.414 0 .75.336.75.75s-.336.75-.75.75"></path></svg>)r"},
                    { .name = "Delete",         .svg = R"r(<svg xmlns="http://www.w3.org/2000/svg" width="20" height="20" fill="none" viewBox="0 0 20 20"><path fill="black" d="M8.249 15.021c-.4 0-.733-.317-.748-.72l-.25-6.5c-.017-.414.307-.763.72-.779H8c.4 0 .733.317.748.72l.25 6.5c.017.414-.307.763-.72.778zM11.751 15.021h-.03c-.413-.016-.737-.365-.72-.779l.25-6.5c.015-.403.348-.72.748-.72h.03c.413.016.737.365.72.779l-.25 6.5c-.015.403-.348.72-.748.72"></path><path fill="black" d="M17 4h-3.5v-.75C13.5 2.01 12.49 1 11.25 1h-2.5C7.51 1 6.5 2.01 6.5 3.25V4H3c-.414 0-.75.336-.75.75s.336.75.75.75h.52l.422 10.342C3.99 17.052 4.98 18 6.19 18h7.62c1.211 0 2.2-.948 2.248-2.158L16.48 5.5H17c.414 0 .75-.336.75-.75S17.414 4 17 4m-9-.75c0-.413.337-.75.75-.75h2.5c.413 0 .75.337.75.75V4H8zm6.56 12.531c-.017.404-.346.719-.75.719H6.19c-.404 0-.733-.315-.75-.719L5.02 5.5h9.96z"></path></svg>)r"},
                    { .name = "Visibility",     .svg=  R"r(<svg xmlns="http://www.w3.org/2000/svg" width="20" height="20" fill="none" viewBox="0 0 20 20"><path fill="black" d="M10.034 16.662C5.143 16.662.75 12.273.75 10.2c0-1.685 2.598-4.55 5.347-5.898 1.201-.606 2.548-.935 3.893-.952 5.158 0 9.26 4.919 9.26 6.85 0 2.072-4.36 6.46-9.216 6.46M10 4.85c-1.11.014-2.231.289-3.231.793l-.008.004C4.155 6.924 2.25 9.398 2.25 10.201c0 1.144 3.68 4.96 7.784 4.96 4.069 0 7.716-3.816 7.716-4.96 0-1.05-3.481-5.35-7.75-5.35m-3.569.124h.01z"></path><path fill="black" d="M9.83 7.644c.222-.637-.474-1.26-1.1-1.01-.474.187-.914.48-1.283.88-1.492 1.614-1.208 4.236.758 5.463 1.2.749 2.797.688 3.94-.147.668-.49 1.103-1.136 1.316-1.834.209-.684-.458-1.25-1.119-.976-.536.223-1.032.128-1.245.067-.57-.165-1.034-.59-1.247-1.144-.167-.436-.163-.893-.02-1.3"></path></svg>)r",},
                    { .name = "VisibilityOff",  .svg=  R"r(<svg xmlns="http://www.w3.org/2000/svg" width="20" height="20" fill="none" viewBox="0 0 20 20"><path fill="black" d="M9.966 16.683C5.11 16.683.75 12.294.75 10.222c0-.92.864-2.36 2.201-3.668.295-.288.77-.284 1.06.011.29.296.285.77-.011 1.061-1.31 1.281-1.75 2.32-1.75 2.596 0 1.144 3.647 4.96 7.716 4.96.604 0 1.231-.084 1.863-.251.398-.102.81.133.917.534.106.4-.134.81-.534.917-.756.2-1.512.3-2.246.3M16.604 13.843c-.204 0-.406-.082-.554-.244-.28-.306-.258-.78.048-1.06 1.14-1.042 1.652-1.982 1.652-2.317 0-.802-1.904-3.277-4.51-4.552-1.01-.51-2.13-.784-3.25-.798-.62 0-1.264.094-1.905.28-.4.109-.814-.116-.929-.513-.115-.398.115-.814.513-.929.775-.225 1.56-.338 2.331-.338 1.356.017 2.704.347 3.907.954 2.742 1.343 5.343 4.21 5.343 5.896 0 .919-.8 2.199-2.14 3.424-.144.132-.326.197-.506.197"></path><g fill="black"><path d="m18.78 17.741-5.778-5.778c.204-.296.358-.615.46-.946.208-.684-.46-1.25-1.12-.976-.495.206-.946.14-1.18.081L9.788 8.75c-.094-.37-.079-.745.04-1.085.223-.637-.473-1.26-1.1-1.01q-.354.14-.677.358L2.28 1.241c-.293-.293-.767-.293-1.06 0s-.293.768 0 1.06l16.5 16.5c.146.147.338.22.53.22s.384-.073.53-.22c.293-.292.293-.767 0-1.06M8.205 12.998c.695.434 1.523.587 2.319.484L6.542 9.501c-.168 1.315.386 2.7 1.663 3.497"></path></g></svg>)r"},
                };

        // clang-format on
        m_icons    = (icon_t const*)icons;

        m_iconData = m_icons[(int)type].svg;
        m_iconLen  = strlen(m_iconData);
    }

    ~IconButton() {}

    lv_obj_t* object() { return m_obj; }

    void setType(Type type) {
        m_type     = type;
        m_iconData = m_icons[(int)type].svg;
        m_iconLen  = strlen(m_iconData);
    }

    void setSize(int32_t w, int32_t h) { lv_obj_set_size(m_obj, w, h); }

    void setChecked(bool checked) { lv_obj_set_state(m_obj, LV_STATE_CHECKED, checked); }

    void setCheckable(bool enable) { lv_obj_set_flag(m_obj, LV_OBJ_FLAG_CHECKABLE, enable); }

    void setPad(float pad) { m_pad = std::clamp<float>(pad, 1, 6); }

    Type type() { return m_type; }

    // 32 20
    void paint(lv_event_t* e) {
        lv_svg_node_t* svg_doc = lv_svg_load_data(m_iconData, m_iconLen / sizeof(char));
        if (!svg_doc) {
            return;
        }

        lv_obj_t* obj = lv_event_get_target_obj(e);
        lv_area_t coords;
        lv_obj_get_coords(obj, &coords);

        float x                   = coords.x1;
        float y                   = coords.y1;
        float w                   = lv_area_get_width(&coords);
        float h                   = lv_area_get_height(&coords);

        lv_layer_t* layer         = lv_event_get_layer(e);
        lv_draw_vector_dsc_t* dsc = lv_draw_vector_dsc_create(layer);
        lv_svg_render_obj_t* list = lv_svg_render_create(svg_doc);

        float base_size           = 20.f;
        float size                = LV_MIN(w, h) - 2 * m_pad;
        if (size < 1)
            size = 1;

        float offset_x = (w - size) / 2.0f;
        float offset_y = (h - size) / 2.0f;
        float scale    = size / base_size;

        lv_matrix_identity(&list->matrix);
        lv_matrix_translate(&list->matrix, x + offset_x, y + offset_y);
        lv_matrix_scale(&list->matrix, scale, scale);

        lv_color32_t color = m_normalColor;
        lv_state_t state   = lv_obj_get_state(m_obj);
        if (state & LV_STATE_PRESSED || state & LV_STATE_CHECKED) {
            color = m_activeColor;
        }

        lv_svg_render_obj_t* it = list->next;
        while (it) {
            it->dsc.stroke_dsc.color = color;
            it->dsc.fill_dsc.color   = color;
            it                       = it->next;
        }

        lv_draw_svg_render(dsc, list);
        lv_draw_vector(dsc);
        lv_svg_render_delete(list);
        lv_draw_vector_dsc_delete(dsc);
        lv_svg_node_delete(svg_doc);
    }

    void update() {
        lv_obj_invalidate(m_obj);
        lv_obj_send_event(m_obj, LV_EVENT_REFRESH, nullptr);
    }
private:
    lv_obj_t* m_obj        = nullptr;
    char const* m_iconData = nullptr;
    int m_iconLen          = 0;
    Type m_type;
    lv_color32_t m_normalColor = lv_color32_make(0, 0, 0, 255);
    lv_color32_t m_activeColor = lv_color32_make(255, 255, 255, 255);
    float m_pad                = 6.0f;
    icon_t const* m_icons;
};  // namespace v1

class Layout
{
public:
    enum class Dir {
        Row,
        Column,
    };

    Layout(Dir dir, lv_obj_t* parent) {
        switch (dir) {
            case Dir::Row:
                m_layout = create_row_layout(parent);
                break;
            case Dir::Column:
                m_layout = create_column_layout(parent);
                break;
            default:
                break;
        }
    }

    ~Layout() {}

    lv_obj_t* object() { return m_layout; }
private:
    inline lv_obj_t* create_layout(lv_obj_t* parent) {
        lv_obj_t* obj = lv_obj_create(parent);
        lv_obj_set_flex_align(obj, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
        lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_obj_set_style_pad_gap(obj, 4, LV_PART_MAIN);
        lv_obj_set_style_margin_all(obj, 6, LV_PART_MAIN);
        lv_obj_set_style_pad_all(obj, 8, LV_PART_MAIN);
        lv_obj_set_style_radius(obj, 4, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_bg_color(obj, lv_color_make(248, 248, 248), LV_PART_MAIN);

        return obj;
    }

    inline lv_obj_t* create_row_layout(lv_obj_t* parent) {
        lv_obj_t* obj = create_layout(parent);
        lv_obj_set_flex_flow(obj, LV_FLEX_FLOW_ROW);
        return obj;
    }

    inline lv_obj_t* create_column_layout(lv_obj_t* parent) {
        lv_obj_t* obj = create_layout(parent);
        lv_obj_set_flex_flow(obj, LV_FLEX_FLOW_COLUMN);
        return obj;
    }

    lv_obj_t* m_layout = nullptr;
};

class Toolbar
{
public:
    Toolbar(lv_obj_t* parent) : m_layout(Layout::Dir::Column, parent) {
        lv_obj_t* layout = m_layout.object();
        lv_obj_align(layout, LV_ALIGN_LEFT_MID, 12, 0);

        auto select = m_buttons.emplace_back(new IconButton(IconButton::Type::Select, layout));
        m_buttons.emplace_back(new IconButton(IconButton::Type::Draw, layout));
        auto vector_draw = m_buttons.emplace_back(new IconButton(IconButton::Type::VectorDraw, layout));
        m_buttons.emplace_back(new IconButton(IconButton::Type::Erase, layout));
        m_buttons.emplace_back(new IconButton(IconButton::Type::Image, layout));
        m_buttons.emplace_back(new IconButton(IconButton::Type::Text, layout));
        m_buttons.emplace_back(new IconButton(IconButton::Type::Start, layout));
        auto delete_object = m_buttons.emplace_back(new IconButton(IconButton::Type::Delete, layout));

        select->setChecked(true);
        delete_object->setCheckable(false);

        lv_obj_add_event_cb(
            vector_draw->object(),
            [](auto e)
            {
                Toolbar* self = (Toolbar*)lv_event_get_user_data(e);
                if (!lv_obj_has_state(lv_event_get_target_obj(e), LV_STATE_CHECKED) && self && self->m_vectorDrawCb) {
                    self->m_vectorDrawCb();
                }
            },
            LV_EVENT_STATE_CHANGED,
            this);
        lv_obj_add_event_cb(
            delete_object->object(),
            [](auto e)
            {
                Toolbar* self = (Toolbar*)lv_event_get_user_data(e);
                if (self->m_deleteCb) {
                    self->m_deleteCb();
                }
            },
            LV_EVENT_CLICKED,
            this);
    }

    std::optional<IconButton::Type> tool() {
        lv_obj_t* layout = m_layout.object();
        uint32_t cnt     = lv_obj_get_child_count(layout);

        lv_obj_t* target = nullptr;
        for (uint32_t i = 0; i < cnt; ++i) {
            lv_obj_t* child = lv_obj_get_child(layout, i);
            if (lv_obj_has_state(child, LV_STATE_CHECKED)) {
                target = child;
                break;
            }
        }

        for (auto& v : m_buttons) {
            if (v->object() == target) {
                m_currentTool = v->type();
            }
        }

        return m_currentTool;
    }

    void setDrawVectorButtonHandle(std::function<void()> const& fn) { m_vectorDrawCb = fn; };

    void setDeleteButtonHandle(std::function<void()> const& fn) { m_deleteCb = fn; }
private:
    std::optional<IconButton::Type> find(lv_obj_t* obj) { return std::nullopt; }
private:
    Layout m_layout;
    std::vector<IconButton*> m_buttons;
    std::optional<IconButton::Type> m_currentTool;

    std::function<void()> m_vectorDrawCb;
    std::function<void()> m_deleteCb;
};

class StatusBar
{
public:
    StatusBar(lv_obj_t* parent) {
        m_obj = lv_label_create(parent);
        lv_obj_set_size(m_obj, LV_PCT(100), LV_PCT(100));
        lv_obj_align(m_obj, LV_ALIGN_TOP_LEFT, 3, 3);
        lv_label_set_text_fmt(m_obj, "Pan (%.2f %.2f) Zoom (%.2f)", 0.0, 0.0, 1.0);
        lv_obj_set_style_text_color(m_obj, lv_color_white(), LV_PART_MAIN);
    }

    void show_status_text(float pan_x, float pan_y, float zoom) { lv_label_set_text_fmt(m_obj, "Pan (%.2f %.2f) Zoom (%.2f)", pan_x, pan_y, zoom); }
private:
    lv_obj_t* m_obj;
};

class ZoomWidget
{
public:
    ZoomWidget(lv_obj_t* parent) : m_layout(Layout::Dir::Row, parent) {
        lv_obj_t* container = m_layout.object();
        lv_obj_set_scroll_dir(container, LV_DIR_NONE);
        lv_obj_set_size(container, LV_PCT(80), 32);
        lv_obj_set_style_pad_gap(container, 16, 0);
        lv_obj_set_style_margin_left(container, 12, LV_PART_MAIN);
        lv_obj_set_style_margin_right(container, 12, LV_PART_MAIN);
        lv_obj_set_flex_align(container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

        auto space = lv_obj_create(container);
        lv_obj_set_size(space, 2, 1);
        lv_obj_set_style_bg_color(space, lv_color_white(), LV_PART_MAIN);

        m_slider = lv_slider_create(container);
        lv_slider_set_range(m_slider, static_cast<int32_t>(m_minScale * 100.0f), static_cast<int32_t>(m_maxScale * 100.0f));
        lv_slider_set_value(m_slider, static_cast<int32_t>(m_currentScale * 100.0f), LV_ANIM_OFF);
        lv_obj_set_style_pad_all(m_slider, -1, LV_PART_KNOB);
        lv_obj_set_style_transform_width(m_slider, 2, LV_PART_KNOB);
        lv_obj_set_style_transform_height(m_slider, 2, LV_PART_KNOB);
        lv_obj_set_style_radius(m_slider, 2, LV_PART_KNOB);

        lv_obj_set_style_bg_color(m_slider, lv_color_hex(0xe0e0e0), LV_PART_MAIN);
        lv_obj_set_style_bg_color(m_slider, lv_color_white(), LV_PART_INDICATOR);
        lv_obj_set_style_border_color(m_slider, lv_color_hex(0xe0e0e0), LV_PART_INDICATOR);
        lv_obj_set_style_border_width(m_slider, 1, LV_PART_INDICATOR);
        lv_obj_set_flex_grow(m_slider, 1);

        lv_obj_add_event_cb(m_slider, [](auto e) { ((ZoomWidget*)lv_event_get_user_data(e))->onValueChanged(e); }, LV_EVENT_VALUE_CHANGED, this);

        m_label = lv_label_create(container);
        lv_obj_set_width(m_label, 100);
        lv_label_set_text_fmt(m_label, "Scale: %.2fx", m_currentScale);
        lv_obj_set_style_text_color(m_label, lv_color_black(), 0);

        lv_obj_align(container, LV_ALIGN_BOTTOM_MID, 0, -32);
    }

    ~ZoomWidget() {}

    void setVisible(bool visible) { lv_obj_set_flag(m_layout.object(), LV_OBJ_FLAG_HIDDEN, !visible); }

    void setZoomHandle(std::function<void(float scale)> cb) { m_callback = cb; }

    void setScale(float scale, bool update_slider = true) {
        scale          = (scale < m_minScale) ? m_minScale : (scale > m_maxScale) ? m_maxScale : scale;
        m_currentScale = scale;

        if (update_slider && m_slider) {
            lv_slider_set_value(m_slider, static_cast<int32_t>(scale * 100.0f), LV_ANIM_OFF);
        }

        if (m_label) {
            lv_label_set_text_fmt(m_label, "Scale: %.2fx", scale);
        }

        if (m_callback) {
            m_callback(scale);
        }
    }

    float getScale() const { return m_currentScale; }
private:
    void onValueChanged(auto e) {
        lv_obj_t* obj = (lv_obj_t*)lv_event_get_target(e);

        int32_t value = lv_slider_get_value(obj);
        float scale   = static_cast<float>(value) * 0.01f;  // 百分制还原

        setScale(scale);
    }
private:
    Layout m_layout;
    lv_obj_t* m_slider{nullptr};
    lv_obj_t* m_label{nullptr};

    float m_minScale                            = 0.1f;
    float m_maxScale                            = 5.0f;
    float m_currentScale                        = 1.0f;
    std::function<void(float scale)> m_callback = nullptr;
};

/* In a single-header file, a wrapper layer is needed to enable calling a transparent object. */
void deleteById(class VectorCanvas* canvas, int id);

class LayerListWidget
{
public:
    LayerListWidget(lv_obj_t* parent) : m_layout(Layout::Dir::Column, parent) {
        lv_obj_set_flex_align(m_layout.object(), LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_width(m_layout.object(), 180);
        lv_obj_set_height(m_layout.object(), LV_PCT(100));
        lv_obj_set_style_bg_color(m_layout.object(), lv_color_hex(0xf8f8f8), LV_PART_MAIN);
        lv_obj_set_style_opa(m_layout.object(), LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_border_color(m_layout.object(), lv_color_hex(0xe0e0e0), LV_PART_MAIN);
        lv_obj_set_style_border_width(m_layout.object(), 1, LV_PART_MAIN);

        lv_obj_t* title = lv_label_create(m_layout.object());
        lv_label_set_text(title, "LayerList");
        lv_obj_set_style_text_color(title, lv_color_make(59, 99, 251), LV_PART_MAIN);

        clear();
    }

    ~LayerListWidget() { delete m_container; }

    void addItem(int id, std::string name = "id") {
        lv_obj_t* layout = lv_obj_create(m_container->object());
        lv_obj_remove_style_all(layout);
        lv_obj_set_size(layout, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_obj_set_flex_flow(layout, LV_FLEX_FLOW_ROW);
        lv_obj_set_style_pad_gap(layout, 6, 0);

        auto label = lv_label_create(layout);
        lv_label_set_text_fmt(label, "%s %d", name.c_str(), id);
        lv_obj_set_width(label, 80);

        auto set_object_style = [&](IconButton* button)
        {
            lv_obj_set_style_pad_all(button->object(), 0, LV_PART_MAIN);
            lv_obj_set_style_margin_all(button->object(), 0, LV_PART_MAIN);
            button->setCheckable(false);
            button->setSize(20, 20);
            button->setPad(2);
        };

        auto visible_icon = new IconButton(IconButton::Type::Visibility, layout);
        set_object_style(visible_icon);

        struct Context {
            int id;
            IconButton* button;
            LayerListWidget* self;
        };

        Context* context = (Context*)lv_malloc_zeroed(sizeof(Context));
        context->button  = visible_icon;
        context->id      = id;
        context->self    = this;

        lv_obj_add_event_cb(
            visible_icon->object(),
            [](auto e)
            {
                auto ctx = (Context*)lv_event_get_user_data(e);
                if (ctx->button->type() == IconButton::Type::Visibility) {
                    ctx->button->setType(IconButton::Type::VisibilityOff);
                    ctx->self->visibleId(false, ctx->id);
                } else {
                    ctx->button->setType(IconButton::Type::Visibility);
                    ctx->self->visibleId(true, ctx->id);
                }
            },
            LV_EVENT_CLICKED,
            context);

        lv_obj_add_event_cb(
            visible_icon->object(),
            [](auto e)
            {
                auto ctx = (Context*)lv_event_get_user_data(e);
                if (ctx) {
                    delete ctx->button;
                    ctx->button = nullptr;
                    delete ctx;
                    ctx = 0;
                    std::println("free memory");
                }
            },
            LV_EVENT_DELETE,
            context);

        auto delete_icon = new IconButton(IconButton::Type::Delete, layout);
        lv_obj_set_style_bg_color(delete_icon->object(), lv_color_make(255, 99, 59), LV_PART_MAIN | LV_STATE_CHECKED);
        lv_obj_set_style_bg_color(delete_icon->object(), lv_color_make(255, 99, 59), LV_PART_MAIN | LV_STATE_PRESSED);
        set_object_style(delete_icon);
        lv_obj_set_user_data(delete_icon->object(), (void*)(intptr_t)id);
        lv_obj_add_event_cb(
            delete_icon->object(),
            [](auto e)
            {
                LayerListWidget* self = (LayerListWidget*)lv_event_get_user_data(e);
                int value             = (int)(intptr_t)lv_obj_get_user_data(lv_event_get_target_obj(e));
                self->deleteId(value);
            },
            LV_EVENT_CLICKED,
            this);
        lv_obj_add_event_cb(
            delete_icon->object(),
            [](auto e)
            {
                auto button = (IconButton*)lv_event_get_user_data(e);
                if (button) {
                    delete button;
                    button = nullptr;
                }
            },
            LV_EVENT_DELETE,
            delete_icon);
    }

    char const* typeToStr(int type) {};

    void clear() {
        if (m_container)
            lv_obj_delete(m_container->object());
        m_container = new Layout(Layout::Dir::Column, m_layout.object());
        lv_obj_set_style_border_width(m_container->object(), 0, LV_PART_MAIN);
        lv_obj_set_width(m_container->object(), LV_PCT(100));
    }

    void deleteId(int id) {
        std::println("delete id = {}", id);
        deleteById(m_canvas, id);
    }

    void visibleId(bool visible, int id) {
        std::println("visible = {} id = {}", visible, id);

        for (auto& shape : *m_shapes) {
            if (shape.id == id) {
                shape.visible = visible;
            }
        }
    }

    void setShapes(std::vector<Shape>* shapes, class VectorCanvas* canvas) {
        m_shapes = shapes;
        m_canvas = canvas;
    }

    void shapesChanged() {
        clear();
        for (auto& shape : *m_shapes) {
            addItem(shape.id, shape.name);
        }
    }
private:
    Layout m_layout;
    lv_obj_t* m_title{nullptr};
    Layout* m_container{nullptr};

    std::vector<Shape>* m_shapes;

    class VectorCanvas* m_canvas;
};

enum class CoordinateSystem {
    LeftTop,  // 默认
    RightTop,
    LeftBottom,
    RightBottom,
    Center
};

static Transform make_coordinate_system(CoordinateSystem cs, float width, float height) {
    Transform transform;
    switch (cs) {
        case CoordinateSystem::LeftTop:
            // 默认，无需处理
            break;
        case CoordinateSystem::RightTop:
            // 原点移到右上 + x 反转
            // transform.setTranslate(width, 0);
            // transform.setScale(-1, 1);
            break;
        case CoordinateSystem::LeftBottom:
            // 原点移到左下 + y 反转
            // transform.setTranslate(0, height);
            // transform.setScale(1, -1);
            break;
        case CoordinateSystem::RightBottom:
            // 原点移到右下 + xy 反转
            // transform.setTranslate(width, height);
            // transform.setScale(-1, -1);
            break;
        case CoordinateSystem::Center:
            // 原点移到中心 + y 向上（更符合数学坐标）
            // transform.setTranslate(width / 2.0f, height / 2.0f);
            // transform.setScale(1, -1);
            break;
    }
    return transform;
}

class VectorCanvas
{
public:
    enum RenderMode {
        EventDriven,  // 使用事件主动刷新渲染
        FrameDriven,  // 使用离屏缓冲区渲染
    };

    explicit VectorCanvas(lv_obj_t* parent) {
        m_canvas = lv_obj_create(parent);
        lv_obj_center(m_canvas);
        lv_obj_set_style_border_width(m_canvas, 3, LV_PART_MAIN);
        lv_obj_set_style_pad_all(m_canvas, 0, LV_PART_MAIN);
        lv_obj_set_style_margin_all(m_canvas, 0, LV_PART_MAIN);
        // If you don't use the layout, you can use the content as described in the following comments.
        // lv_obj_set_size(m_canvas, LV_PCT(100), LV_PCT(100));
        // lv_obj_set_width(m_canvas, LV_PCT(100));
        lv_obj_set_flex_grow(m_canvas, 1);
        lv_obj_set_height(m_canvas, LV_PCT(100));
        lv_obj_update_layout(m_canvas);

        lv_obj_add_event_cb(
            m_canvas,
            [](auto e)
            {
                auto self         = static_cast<VectorCanvas*>(lv_event_get_user_data(e));
                lv_layer_t* layer = lv_event_get_layer(e);
                VectorPainter painter(layer);
                self->paint(&painter);
            },
            LV_EVENT_DRAW_MAIN,
            this);

        m_width  = lv_obj_get_width(m_canvas);
        m_height = lv_obj_get_height(m_canvas);
        m_mode   = EventDriven;

        setupInit();
    }

    explicit VectorCanvas(int width, int height, lv_obj_t* parent) {
        m_canvas                = lv_canvas_create(parent);
        lv_draw_buf_t* draw_buf = lv_draw_buf_create(width, height, LV_COLOR_FORMAT_NATIVE_WITH_ALPHA, LV_STRIDE_AUTO);
        lv_draw_buf_clear(draw_buf, NULL);
        lv_canvas_set_draw_buf(m_canvas, draw_buf);
        lv_obj_add_event_cb(
            m_canvas,
            [](auto e)
            {
                lv_obj_t* obj           = lv_event_get_target_obj(e);
                lv_draw_buf_t* draw_buf = lv_canvas_get_draw_buf(obj);
                lv_draw_buf_destroy(draw_buf);
            },
            LV_EVENT_DELETE,
            nullptr);

        m_width  = width;
        m_height = height;
        m_mode   = FrameDriven;

        setupInit();
    }

    ~VectorCanvas() { lv_obj_delete(m_canvas); }

    void setupInit() {
        lv_obj_set_scroll_dir(m_canvas, LV_DIR_NONE);

        lv_obj_update_layout(m_canvas);
        m_x = lv_obj_get_x(m_canvas);
        m_y = lv_obj_get_y(m_canvas);

        lv_obj_add_event_cb(m_canvas, [](auto e) { (static_cast<VectorCanvas*>(lv_event_get_user_data(e)))->event(e); }, LV_EVENT_PRESSED, this);
        lv_obj_add_event_cb(m_canvas, [](auto e) { (static_cast<VectorCanvas*>(lv_event_get_user_data(e)))->event(e); }, LV_EVENT_PRESSING, this);
        lv_obj_add_event_cb(m_canvas, [](auto e) { (static_cast<VectorCanvas*>(lv_event_get_user_data(e)))->event(e); }, LV_EVENT_RELEASED, this);
    }

    lv_obj_t* object() { return m_canvas; }

    void setWidth(int32_t w) { lv_obj_set_width(m_canvas, w); }

    void setHeight(int32_t h) { lv_obj_set_height(m_canvas, h); }

    void setToolBar(Toolbar* bar) {
        m_toolbar = bar;
        m_toolbar->setDeleteButtonHandle([&]() { clear(); });
        m_toolbar->setDrawVectorButtonHandle(
            [&]()
            {
                if (m_toolbar && m_toolbar->tool() == IconButton::Type::VectorDraw && m_activeShape) {
                    m_activeShape = false;
                    m_shapes.emplace_back(m_cacheShape);
                    shapesChanged();
                }
            });
    }

    void setStatusBar(StatusBar* bar) { m_statusBar = bar; }

    void setZoomWidget(ZoomWidget* widget) {
        m_zoomWidget = widget;

        m_zoomWidget->setZoomHandle(
            [&](float scale)
            {
                m_scale = scale;
                if (m_statusBar) {
                    m_statusBar->show_status_text(m_offset.x, m_offset.y, m_scale);
                }
            });
    }

    // 屏幕触摸点按下&移动&弹起事件
    virtual void event(lv_event_t* e) {
        lv_indev_t* indev = lv_indev_active();
        if (indev == nullptr)
            return;

        lv_point_t point;
        lv_indev_get_point(indev, &point);
        lv_fpoint_t fpoint(point.x, point.y);

        lv_event_code_t code = lv_event_get_code(e);
        switch (code) {
            case LV_EVENT_PRESSED:
                onPressed(fpoint);
                break;
            case LV_EVENT_PRESSING:
                onMove(fpoint);
                break;
            case LV_EVENT_RELEASED:
                onRelease(fpoint);
                break;
            default:
                break;
        }
    }

    void onPressed(lv_fpoint_t point) {
        std::println("pressed ({}, {})", point.x, point.y);
        if (m_toolbar && m_toolbar->tool() == IconButton::Type::Select) {
            m_lastPressed = point;
        } else if (m_toolbar && m_toolbar->tool() == IconButton::Type::Draw) {
            m_cacheShape.reset();
            m_cacheShape.type    = ShapeType::Vector;
            m_cacheShape.content = GeometryData{};
            m_cacheShape.id      = ++m_incrementId;
            m_cacheShape.name    = "pencil";

            auto& geo            = std::get<GeometryData>(m_cacheShape.content);
            geo.type             = GeometryType::Path;
            point.x -= m_offset.x;
            point.y -= m_offset.y;
            geo.points.push_back(point);
        } else if (m_toolbar && m_toolbar->tool() == IconButton::Type::VectorDraw) {
            if (!m_activeShape) {
                m_cacheShape.reset();
                m_cacheShape.type    = ShapeType::Vector;
                m_cacheShape.content = GeometryData{};
                m_cacheShape.id      = ++m_incrementId;
                m_cacheShape.name    = "pen";
            }

            auto& geo = std::get<GeometryData>(m_cacheShape.content);
            geo.type  = GeometryType::Bezier;

            point.x -= m_offset.x;
            point.y -= m_offset.y;

            m_bezierControlNet.a = {point.x, point.y};
            geo.beziers.emplace_back(BezierAnchorPoint{point, std::nullopt, std::nullopt});

            m_activeShape = true;
        } else {
        }
    }

    void onMove(lv_fpoint_t point) {
        std::println("move ({}, {})", point.x, point.y);

        if (m_toolbar && m_toolbar->tool() == IconButton::Type::Select) {
            float dx = point.x - m_lastPressed.x;
            float dy = point.y - m_lastPressed.y;

            m_offset.x += dx;
            m_offset.y += dy;

            m_lastPressed = point;
        } else if (m_toolbar && m_toolbar->tool() == IconButton::Type::Draw) {
            auto& geo = std::get<GeometryData>(m_cacheShape.content);
            point.x -= m_offset.x;
            point.y -= m_offset.y;
            geo.points.push_back(point);
        } else if (m_toolbar && m_toolbar->tool() == IconButton::Type::VectorDraw && m_activeShape) {
            point.x -= m_offset.x;
            point.y -= m_offset.y;
            m_bezierControlNet.b       = {point.x, point.y};
            m_bezierControlNet.visible = true;

            auto& geo                  = std::get<GeometryData>(m_cacheShape.content);
            if (geo.beziers.size() == 1) {
                auto& v4 = geo.beziers[geo.beziers.size() - 1];
                v4.out   = {m_bezierControlNet.b.first, m_bezierControlNet.b.second};

            } else if (geo.beziers.size() >= 2) {
                auto& v1       = geo.beziers[geo.beziers.size() - 2];  // p
                auto& v4       = geo.beziers[geo.beziers.size() - 1];  // c

                auto p1        = v1.p;
                auto p4        = v4.p;

                lv_fpoint_t p2 = {m_bezierControlNet.reverse().first, m_bezierControlNet.reverse().second};
                lv_fpoint_t p3 = {m_bezierControlNet.b.first, m_bezierControlNet.b.second};

                v4.in          = p2;
                v4.out         = p3;
            }
        } else if (m_toolbar && m_toolbar->tool() == IconButton::Type::Erase) {
            point.x -= m_offset.x;
            point.y -= m_offset.y;
            erase(point);
        }

        if (m_statusBar) {
            m_statusBar->show_status_text(m_offset.x, m_offset.y, m_scale);
        }
    }

    void onRelease(lv_fpoint_t point) {
        std::println("release ({}, {})", point.x, point.y);
        if (m_toolbar && m_toolbar->tool() == IconButton::Type::Select) {
        } else if (m_toolbar && m_toolbar->tool() == IconButton::Type::Draw) {
            auto& geo = std::get<GeometryData>(m_cacheShape.content);
            m_shapes.push_back(m_cacheShape);
            m_cacheShape.visible = false;
            shapesChanged();
        } else if (m_toolbar && m_toolbar->tool() == IconButton::Type::VectorDraw) {
            m_bezierControlNet.visible = false;
        }
    }

    // 绘制&强制重绘
    void paint(VectorPainter* painter) {
        lv_area_t rect;
        lv_obj_get_content_coords(m_canvas, &rect);
        painter->clear(rect);

        if constexpr (false) {
            float cx = m_width / 2;
            float cy = m_height / 2;

            painter->set_stroke_color(lv_palette_main(LV_PALETTE_RED));
            painter->draw_line({0, cy}, {m_width, cy});
            painter->set_fill_color(lv_palette_main(LV_PALETTE_RED));
            painter->draw_arrow({
                {      0, cy},
                {m_width, cy}
            });
            painter->set_stroke_color(lv_palette_main(LV_PALETTE_GREEN));
            painter->draw_line({cx, 0}, {cx, m_height});
            painter->set_fill_color(lv_palette_main(LV_PALETTE_GREEN));
            painter->draw_arrow({
                {cx,        0},
                {cx, m_height}
            });
            painter->set_fill_color(lv_palette_main(LV_PALETTE_GREY));
            painter->draw_point({cx, cy}, 5);

            painter->draw_dash_line({0, 0}, {cx, cy});
        }

        // Transform
        painter->translate(m_offset.x, m_offset.y);

        // main draw
        painter->set_stroke_width(3);
        painter->set_stroke_color(lv_color_hex(0x00ff00));
        if (m_cacheShape.visible) {
            if (m_cacheShape.type == ShapeType::Vector) {
                auto& geo = std::get<GeometryData>(m_cacheShape.content);
                if (geo.type == GeometryType::Path) {
                    painter->draw_polyline(geo.points);
                }
            }
        }

        if (m_bezierControlNet.visible) {
            auto [x, y]   = m_bezierControlNet.a;  // p
            auto [x1, y1] = m_bezierControlNet.b;
            auto [x2, y2] = m_bezierControlNet.reverse();

            painter->set_stroke_color(lv_color_hex(0xffff00));
            painter->draw_dash_line(lv_fpoint_t{x1, y1}, lv_fpoint_t{x2, y2});
            painter->set_fill_color(lv_color_hex(0x00ff00));
            painter->draw_point(lv_fpoint_t{x, y}, 5);
            painter->set_fill_color(lv_color_hex(0xffff00));
            painter->draw_point(lv_fpoint_t{x1, y1}, 5);
            painter->draw_point(lv_fpoint_t{x2, y2}, 5);
        }

        auto draw_bezier_shape = [&](Shape& shape)
        {
            auto& geo = std::get<GeometryData>(shape.content);
            if (geo.beziers.size() > 0) {
                painter->set_fill_color(lv_color_hex(0x00ff00));
                painter->draw_point(geo.beziers[0].p, 5);
            }
            if (geo.beziers.size() >= 2) {
                VectorPath draft;
                for (int i = 1; i < geo.beziers.size(); i++) {
                    auto p1 = geo.beziers[i - 1].p;
                    auto p4 = geo.beziers[i].p;

                    auto p2 = p1;
                    if (geo.beziers[i - 1].out) {
                        p2 = *geo.beziers[i - 1].out;
                    }

                    auto p3 = p4;
                    if (geo.beziers[i].in) {
                        p3 = *geo.beziers[i].in;
                    }

                    draft.move_to(p1);
                    draft.cubic_to(p2, p3, p4);

                    painter->set_fill_color(lv_color_hex(0x00ff00));
                    painter->draw_point(p4, 5);

                    if (shape.editor) {
                        painter->set_stroke_color(lv_color_hex(0xffff00));
                        painter->draw_dash_line(p1, p2);
                        painter->draw_dash_line(p4, p3);
                        painter->set_fill_color(lv_color_hex(0xffff00));
                        painter->draw_point(p2, 5);
                        painter->draw_point(p3, 5);
                    }
                }  // end for

                painter->set_stroke_color(lv_color_hex(0x00ff00));
                painter->set_fill_color(std::nullopt);
                painter->add_path(draft);
            }
        };

        painter->set_stroke_color(lv_color_hex(0x00ff00));
        if (m_activeShape && m_cacheShape.type == ShapeType::Vector) {
            draw_bezier_shape(m_cacheShape);
        }

        // draw shape
        for (auto& shape : m_shapes) {
            if (shape.visible) {
                if (shape.type == ShapeType::Vector) {
                    auto& geo = std::get<GeometryData>(shape.content);
                    if (geo.type == GeometryType::Path) {
                        painter->draw_polyline(geo.points);
                    } else if (geo.type == GeometryType::Bezier) {
                        draw_bezier_shape(shape);
                    }
                }
            }
        }
    }

    void requestPaint() {
        if (m_mode == EventDriven) {
            lv_obj_invalidate(m_canvas);
            lv_obj_send_event(m_canvas, LV_EVENT_REFRESH, nullptr);
        } else if (m_mode == FrameDriven) {
            lv_layer_t layer;
            lv_canvas_init_layer(m_canvas, &layer);

            VectorPainter painter(&layer);
            paint(&painter);

            lv_canvas_finish_layer(m_canvas, &layer);
        }
    }

    void erase(lv_fpoint_t pos) {
        float constexpr radius = 5.0f;
        float const r2         = radius * radius;

        std::erase_if(m_shapes,
                      [&](Shape const& shape)
                      {
                          if (!shape.visible)
                              return false;

                          if (shape.type == ShapeType::Vector) {
                              auto& geo = std::get<GeometryData>(shape.content);

                              if (geo.type == GeometryType::Path) {
                                  return std::ranges::any_of(geo.points,
                                                             [&](auto const& p)
                                                             {
                                                                 float dx = p.x - pos.x;
                                                                 float dy = p.y - pos.y;
                                                                 return dx * dx + dy * dy < r2;
                                                             });
                              } else if (geo.type == GeometryType::Bezier) {
                                  // todo
                              }
                          }
                          return false;
                      });

        shapesChanged();
    }

    void clear() {
        m_shapes.clear();
        m_cacheShape.reset();
        m_activeShape = false;

        shapesChanged();
    }

    void deleteById(int id) {
        std::erase_if(m_shapes,
                      [&](Shape const& shape)
                      {
                          if (shape.id == id)
                              return true;
                          return false;
                      });

        shapesChanged();
    }

    void shapesChanged() {
        for (auto& fn : notify) {
            fn();
        }
    }

    // 平移&缩放
    lv_fpoint_t offset() const { return m_offset; }

    float scale() const { return m_scale; }

    void pan(float dx, float dy) {}

    void zoom(float factor, float screen_cx, float screen_cy) {}

    // 坐标转换
    lv_fpoint_t worldToScreen(lv_fpoint_t const& world) const {}

    lv_fpoint_t screenToWorld(lv_fpoint_t const& screen) const {}

    Transform worldToScreenTransform() const {}

    void setShapes(std::vector<Shape> shapes) { m_shapes = shapes; }

    // 拾取，返回 shape id
    std::optional<uint64_t> pick(lv_fpoint_t screen_pt) const {}

    std::vector<uint64_t> pickRect(lv_area_t screen_rect) const {}

    std::vector<Shape>* getShapes() { return &m_shapes; }

    std::vector<std::function<void()>> notify;
private:
    lv_obj_t* m_canvas{nullptr};
    float m_x{0};
    float m_y{0};
    float m_width{0};
    float m_height{0};

    std::vector<Shape> m_shapes;
    inline static uint64_t m_incrementId = 0;
    RenderMode m_mode;

    // 平移、缩放
    lv_fpoint_t m_offset{0, 0};
    float m_scale{1.0f};

    lv_fpoint_t m_lastPressed = {0, 0};

    struct BezierControlNet {
        using Vec2 = std::pair<float, float>;
        Vec2 a     = {0, 0};
        Vec2 b     = {0, 0};

        Vec2 reverse() {
            Vec2 v    = {b.first - a.first, b.second - a.second};
            float len = std::hypot(v.first, v.second);
            Vec2 n    = len > 0 ? Vec2{v.first / len, v.second / len} : Vec2{0, 0};
            return {a.first + -n.first * len, a.second + -n.second * len};
        }

        bool visible = false;
    };

    BezierControlNet m_bezierControlNet;

    Shape m_cacheShape;
    bool m_activeShape{false};

    // action
    Toolbar* m_toolbar{nullptr};
    StatusBar* m_statusBar{nullptr};
    ZoomWidget* m_zoomWidget{nullptr};
};

inline void deleteById(VectorCanvas* canvas, int id) {
    canvas->deleteById(id);
}
