#pragma once

#include "resource.h"

#include <functional>
#include <iostream>
#include <linalg.h>
#include <memory>
#include <cfloat>

using namespace linalg::aliases;

namespace cg::renderer {
    template<typename VB, typename RT>
    class rasterizer {
    public:
        rasterizer() {};

        ~rasterizer() {};

        void set_render_target(
                std::shared_ptr<resource<RT>> in_render_target,
                std::shared_ptr<resource<float>> in_depth_buffer = nullptr);

        void clear_render_target(
                const RT &in_clear_value, const float in_depth = FLT_MAX);

        void set_vertex_buffer(std::shared_ptr<resource<VB>> in_vertex_buffer);

        void set_index_buffer(std::shared_ptr<resource<unsigned int>> in_index_buffer);

        void set_viewport(size_t in_width, size_t in_height);

        void draw(size_t num_vertexes, size_t vertex_offset);

        std::function<std::pair<float4, VB>(float4 vertex, VB vertex_data)> vertex_shader;
        std::function<cg::color(const VB &vertex_data, const float z)> pixel_shader;

    protected:
        std::shared_ptr<cg::resource<VB>> vertex_buffer;
        std::shared_ptr<cg::resource<unsigned int>> index_buffer;
        std::shared_ptr<cg::resource<RT>> render_target;
        std::shared_ptr<cg::resource<float>> depth_buffer;

        size_t width = 1920;
        size_t height = 1080;

        float edge_function(float2 a, float2 b, float2 c);

        bool depth_test(float z, size_t x, size_t y);

        float lin_interp(float a, float b, float m);
    };

    template<typename VB, typename RT>
    inline void rasterizer<VB, RT>::set_render_target(
            std::shared_ptr<resource<RT>> in_render_target,
            std::shared_ptr<resource<float>> in_depth_buffer) {
        if (in_render_target) {
            render_target = in_render_target;
        }
        if (in_depth_buffer) {
            depth_buffer = in_depth_buffer;
        }
    }

    template<typename VB, typename RT>
    inline void rasterizer<VB, RT>::set_viewport(size_t in_width, size_t in_height) {
        width = in_width;
        height = in_height;
    }

    template<typename VB, typename RT>
    inline void rasterizer<VB, RT>::clear_render_target(
            const RT &in_clear_value, const float in_depth) {
        if (render_target != nullptr) {
            for (int i = 0; i < render_target->get_number_of_elements(); ++i) {
                render_target->item(i) = in_clear_value;
            }
        }
        if (depth_buffer != nullptr) {
            for (int i = 0; i < depth_buffer->get_number_of_elements(); ++i) {
                depth_buffer->item(i) = in_depth;
            }
        }
    }

    template<typename VB, typename RT>
    inline void rasterizer<VB, RT>::set_vertex_buffer(
            std::shared_ptr<resource<VB>> in_vertex_buffer) {
        vertex_buffer = in_vertex_buffer;
    }

    template<typename VB, typename RT>
    inline void rasterizer<VB, RT>::set_index_buffer(
            std::shared_ptr<resource<unsigned int>> in_index_buffer) {
        index_buffer = in_index_buffer;
    }

    template<typename VB, typename RT>
    inline float rasterizer<VB, RT>::lin_interp(float a, float b, float m) {
        return (1 - m) * a + m * b;
    }

    template<typename VB, typename RT>
    inline void rasterizer<VB, RT>::draw(size_t num_vertexes, size_t vertex_offset) {
        for (size_t vertex_ind = vertex_offset; vertex_ind < num_vertexes + vertex_offset;) {
            std::vector<VB> vertices(3);
            vertices[0] = vertex_buffer->item(index_buffer->item(vertex_ind++));
            vertices[1] = vertex_buffer->item(index_buffer->item(vertex_ind++));
            vertices[2] = vertex_buffer->item(index_buffer->item(vertex_ind++));
            for (auto &vertex: vertices) {
                float4 cords{vertex.x, vertex.y, vertex.z, 1.0f};
                auto processed = vertex_shader(cords, vertex);
                vertex.x = processed.first.x / processed.first.w;
                vertex.y = processed.first.y / processed.first.w;
                vertex.z = processed.first.z / processed.first.w;

                vertex.x = (vertex.x + 1) * width / 2.0f;
                vertex.y = (-vertex.y + 1) * height / 2.0f;
            }
            float2 vertex_a = float2{vertices[0].x, vertices[0].y};
            float2 vertex_b = float2{vertices[1].x, vertices[1].y};
            float2 vertex_c = float2{vertices[2].x, vertices[2].y};

            float edge = edge_function(vertex_a, vertex_b, vertex_c);

            float2 min_vertex = min(vertex_a, min(vertex_b, vertex_c));
            float2 bounding_box_begin = round(clamp(min_vertex, float2{0, 0}, float2{static_cast<float>(width - 1),
                                                                                     static_cast<float>(height - 1)}));
            float2 max_vertex = max(vertex_a, max(vertex_b, vertex_c));
            float2 bounding_box_end = round(clamp(max_vertex, float2{0, 0}, float2{static_cast<float>(width - 1),
                                                                                   static_cast<float>(height - 1)}));
            for (float x = bounding_box_begin.x; x <= bounding_box_end.x; x += 1.0f) {
                for (float y = bounding_box_begin.y; y <= bounding_box_end.y; y += 1.0f) {
                    float2 point(x, y);
                    float edge0 = edge_function(vertex_a, vertex_b, point);
                    float edge1 = edge_function(vertex_b, vertex_c, point);
                    float edge2 = edge_function(vertex_c, vertex_a, point);
                    if (edge0 >= 0.0f && edge1 >= 0.0f && edge2 >= 0.0f) {
/*                        float x0 = vertex_a.x;
                        float y0 = vertex_a.y;
                        float x1 = vertex_b.x;
                        float y1 = vertex_b.y;
                        float x2 = vertex_c.x;
                        float y2 = vertex_c.y;

                        float intersect_x = ((y2 - y0) * (x2 - x0) * (x - x2) - x2 * (y - y2) * (x1 - x0) +
                                             x0 * (y1 - y0) * (x - x2)) / ((y1 - y0) * (x - x2) - (y - y2) * (x1 - x0));
                        float intersect_z = lin_interp(vertices[0].z, vertices[1].z, (intersect_x - x0) / (x1 - x0));
                        float depth = lin_interp(vertices[2].z, intersect_z,
                                                 (x - vertex_c.x) / (intersect_x - vertex_c.x));*/
                        float u = edge1 / edge;
                        float v = edge2 / edge;
                        float w = edge0 / edge;
                        float depth = u * vertices[0].z + v * vertices[1].z + w * vertices[2].z;
                        size_t u_x = static_cast<size_t>(x);
                        size_t u_y = static_cast<size_t>(y);
                        if (depth_test(depth, u_x, u_y)) {
                            auto v = vertices[0];
                            v.x = x;
                            v.y = y;
                            auto pixel_result = pixel_shader(v, depth);
                            render_target->item(u_x, u_y) = RT::from_color(pixel_result);
                            if (depth_buffer != nullptr) {
                                depth_buffer->item(u_x, u_y) = depth;
                            }
                        }

                    }
                }
            }
        }
    }

    template<typename VB, typename RT>
    inline float
    rasterizer<VB, RT>::edge_function(float2 a, float2 b, float2 c) {
        return (c.x - a.x) * (b.y - a.y) - (c.y - a.y) * (b.x - a.x);
    }

    template<typename VB, typename RT>
    inline bool rasterizer<VB, RT>::depth_test(float z, size_t x, size_t y) {
        if (!depth_buffer) {
            return true;
        }
        return depth_buffer->item(x, y) > z;
    }

}// namespace cg::renderer