#pragma once

#include "utils/error_handler.h"

#include <algorithm>
#include <linalg.h>
#include <vector>


using namespace linalg::aliases;

namespace cg {
    template<typename T>
    class resource {
    public:
        resource(size_t size);

        resource(size_t x_size, size_t y_size);

        ~resource();

        const T *get_data();

        T &item(size_t item);

        T &item(size_t x, size_t y);

        size_t get_size_in_bytes() const;

        size_t get_number_of_elements() const;

        size_t get_stride() const;

    private:
        std::vector<T> data;
        size_t item_size = sizeof(T);
        size_t stride;
    };

    template<typename T>
    inline resource<T>::resource(size_t size): stride(0), data(size) {
    }

    template<typename T>
    inline resource<T>::resource(size_t x_size, size_t y_size): stride(x_size), data(x_size * y_size) {

    }

    template<typename T>
    inline resource<T>::~resource() {
    }

    template<typename T>
    inline const T *resource<T>::get_data() {
        return data.data();
    }

    template<typename T>
    inline T &resource<T>::item(size_t item) {
        return data[item];
    }

    template<typename T>
    inline T &resource<T>::item(size_t x, size_t y) {
        return data[y * stride + x];
    }

    template<typename T>
    inline size_t resource<T>::get_size_in_bytes() const {
        return item_size * get_number_of_elements();
    }

    template<typename T>
    inline size_t resource<T>::get_number_of_elements() const {
        return data.size();
    }

    template<typename T>
    inline size_t resource<T>::get_stride() const {
        return stride;
    }

    struct color {
        static color from_float3(const float3 &in) {
            return {in.x, in.y, in.z};
        };

        float3 to_float3() const {
            return {r, g, b};
        }

        float r;
        float g;
        float b;
    };

    struct unsigned_color {
        static unsigned_color from_color(const color &color) {
            return unsigned_color{static_cast<uint8_t>(color.r * 255.f),
                                  static_cast<uint8_t>(color.g * 255.f),
                                  static_cast<uint8_t>(color.b * 255.f)};
        };

        static unsigned_color from_float3(const float3 &color) {
            return {static_cast<uint8_t>(color.x * 255.f),
                    static_cast<uint8_t>(color.y * 255.f),
                    static_cast<uint8_t>(color.z * 255.f)};
        };

        float3 to_float3() const {
            return {static_cast<float>(r) / 255.f, static_cast<float>(g) / 255.f, static_cast<float>(b) / 255.f};
        };
        uint8_t r;
        uint8_t g;
        uint8_t b;
    };


    struct vertex {
        float x, y, z, nx, ny, nz, u, v, ambient_r, ambient_g, ambient_b, diffuse_r, diffuse_g, diffuse_b, emissive_r, emissive_g, emissive_b;
    };

}// namespace cg