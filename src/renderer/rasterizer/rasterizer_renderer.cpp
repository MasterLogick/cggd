#include "rasterizer_renderer.h"

#include "utils/resource_utils.h"

namespace cg {
    void renderer::rasterization_renderer::init() {
        rasterizer = std::make_shared<cg::renderer::rasterizer<cg::vertex, cg::unsigned_color>>();
        rasterizer->set_viewport(settings->width, settings->height);
        render_target = std::make_shared<resource<unsigned_color>>(settings->width, settings->height);
        depth_buffer = std::make_shared<resource<float>>(settings->width, settings->height);
        rasterizer->set_render_target(render_target, depth_buffer);
        model = std::make_shared<cg::world::model>();
        model->load_obj(settings->model_path);
        camera = std::make_shared<cg::world::camera>();
        camera->set_height(static_cast<float>(settings->height));
        camera->set_width(static_cast<float>(settings->width));
        camera->set_position(
                float3{settings->camera_position[0], settings->camera_position[1], settings->camera_position[2]});
        camera->set_phi(settings->camera_phi);
        camera->set_theta(settings->camera_theta);
        camera->set_angle_of_view(settings->camera_angle_of_view);
        camera->set_z_near(settings->camera_z_near);
        camera->set_z_far(settings->camera_z_far);
    }

    void renderer::rasterization_renderer::render() {
        auto start = std::chrono::high_resolution_clock::now();
        rasterizer->clear_render_target({0, 0, 0});
        float4x4 matrix = mul(camera->get_projection_matrix(), camera->get_view_matrix(), model->get_world_matrix());
        rasterizer->vertex_shader = [&matrix](float4 vertex, cg::vertex vertexData) -> std::pair<float4, cg::vertex> {
            auto processed = mul(matrix, vertex);
            return std::make_pair(processed, vertexData);
        };
        rasterizer->pixel_shader = [](cg::vertex vertex_data, float z) {
            z = std::abs(std::cos(z * 100000.0f));
            return cg::color{
                    vertex_data.ambient_r * z, vertex_data.ambient_g * z, vertex_data.ambient_b * z
            };
        };
        for (int i = 0; i < model->get_index_buffers().size(); ++i) {
            rasterizer->set_vertex_buffer(model->get_vertex_buffers()[i]);
            rasterizer->set_index_buffer(model->get_index_buffers()[i]);
            rasterizer->draw(model->get_index_buffers()[i]->get_number_of_elements(), 0);
        }
        auto end = std::chrono::high_resolution_clock::now();
        std::cout << "Render time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
                  << "ms" << std::endl;
        cg::utils::save_resource(*render_target, settings->result_path);
    }

    void renderer::rasterization_renderer::destroy() {}

    void renderer::rasterization_renderer::update() {}
}