#pragma once

#include "pch.h"

namespace graphics::v1
{
    struct Transform {
        float tx = 0.f;
        float ty = 0.f;
        float sx = 1.f;
        float sy = 1.f;
        // --- 旋转（角度制，顺时针为正，与 LVGL 一致）---
        float deg     = 0.f;
        float pivot_x = 0.5f;
        float pivot_y = 0.5f;

        lv_matrix_t build(lv_obj_t const* obj) const {
            float w  = static_cast<float>(lv_obj_get_width(const_cast<lv_obj_t*>(obj)));
            float h  = static_cast<float>(lv_obj_get_height(const_cast<lv_obj_t*>(obj)));

            float cx = w * pivot_x;
            float cy = h * pivot_y;

            lv_matrix_t m;
            lv_matrix_identity(&m);
            lv_matrix_translate(&m, tx, ty);
            lv_matrix_translate(&m, cx, cy);
            lv_matrix_rotate(&m, deg);
            lv_matrix_scale(&m, sx, sy);
            lv_matrix_translate(&m, -cx, -cy);

            return m;
        }

        Transform& apply(lv_obj_t* obj) {
            if (obj == nullptr)
                return *this;
            lv_matrix_t m = build(obj);
            lv_obj_set_transform(obj, &m);
            return *this;
        }

        Transform& reset() {
            tx = ty = 0.f;
            sx = sy = 1.f;
            deg     = 0.f;
            pivot_x = pivot_y = 0.5f;
            return *this;
        }

        static Transform from_translate(float x, float y) {
            Transform t;
            t.tx = x;
            t.ty = y;
            return t;
        }

        static Transform from_rotate(float deg) {
            Transform t;
            t.deg = deg;
            return t;
        }

        static Transform from_scale(float s) {
            Transform t;
            t.sx = t.sy = s;
            return t;
        }
    };
}  // namespace graphics::v1
