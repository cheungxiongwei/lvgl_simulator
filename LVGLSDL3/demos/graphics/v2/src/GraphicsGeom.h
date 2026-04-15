#pragma once

#include "lvgl.h"
#include <vector>
#include <print>

// 点、线、多线段、弧形、三角形、圆形、矩形、多边形
namespace geom
{
    template <typename Scalar = float>
    struct point_t {
        Scalar x;
        Scalar y;

        point_t operator+(point_t const& other) const { return point_t{x + other.x, y + other.y}; }

        point_t operator-(point_t const& other) const { return point_t{x - other.x, y - other.y}; }

        point_t operator*(Scalar scalar) const { return point_t{x * scalar, y * scalar}; }

        point_t operator/(Scalar scalar) const { return point_t{x / scalar, y / scalar}; }

        point_t& operator+=(point_t const& other) {
            x += other.x;
            y += other.y;
            return *this;
        }

        point_t& operator-=(point_t const& other) {
            x -= other.x;
            y -= other.y;
            return *this;
        }

        point_t& operator*=(Scalar scalar) {
            x *= scalar;
            y *= scalar;
            return *this;
        }

        point_t& operator/=(Scalar scalar) {
            x /= scalar;
            y /= scalar;
            return *this;
        }

        point_t midpoint(point_t const& other) const {
            return point_t{(x + other.x) / static_cast<Scalar>(2), (y + other.y) / static_cast<Scalar>(2)};
        }

        static point_t midpoint(point_t const& a, point_t const& b) {
            return point_t{(a.x + b.x) / static_cast<Scalar>(2), (a.y + b.y) / static_cast<Scalar>(2)};
        }
    };

    template <typename Scalar = float>
    struct line_t {
        point_t<Scalar> start;
        point_t<Scalar> end;

        Scalar length() const { return std::hypot(end.x - start.x, end.y - start.y); }

        point_t<Scalar> midpoint() const { return start.midpoint(end); }

        bool operator==(line_t<Scalar> const& other) const {
            return start.x == other.start.x && start.y == other.start.y && end.x == other.end.x && end.y == other.end.y;
        }

        bool operator!=(line_t<Scalar> const& other) const { return !(*this == other); }
    };

    template <typename Scalar = float>
    struct polyline_t {
        std::vector<point_t<Scalar>> points;
    };

    template <typename Scalar = float>
    struct arc_t {
        Scalar radius;
        Scalar start_angle;  // in radians
        Scalar end_angle;    // in radians
    };

    template <typename Scalar = float>
    struct triangle_t {
        point_t<Scalar> a;
        point_t<Scalar> b;
        point_t<Scalar> c;
    };

    template <typename Scalar = float>
    struct circle_t {
        Scalar radius;
    };

    template <typename Scalar = float>
    struct rect_t {
        point_t<Scalar> top_left;
        Scalar width;
        Scalar height;
    };

    template <typename Scalar = float>
    struct polygon_t {
        std::vector<point_t<Scalar>> vertices;
    };

    template <typename Scalar = float>
    struct bezier_point_t {
        point_t<Scalar> position;
        point_t<Scalar> in_handle;
        point_t<Scalar> out_handle;
        bool has_in  = false;
        bool has_out = false;
    };

    template <typename Scalar = float>
    struct curve_t {
        std::vector<bezier_point_t<Scalar>> points;
        bool closed = false;
    };

}  // namespace geom
