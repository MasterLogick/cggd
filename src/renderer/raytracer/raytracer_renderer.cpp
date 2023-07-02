#include "raytracer_renderer.h"

#include "utils/resource_utils.h"

#include <iostream>


void cg::renderer::ray_tracing_renderer::init() {
    raytracer = std::make_shared<cg::renderer::raytracer<cg::vertex, cg::unsigned_color>>();
    raytracer->set_viewport(settings->width, settings->height);
    render_target = std::make_shared<resource<unsigned_color>>(settings->width, settings->height);
    raytracer->set_render_target(render_target);
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

    raytracer->set_vertex_buffers(model->get_vertex_buffers());
    raytracer->set_index_buffers(model->get_index_buffers());

    lights.push_back({float3{-0.24f, 1.97f, 0.16f}, float3{0.78f, 0.78f, 0.78f} / 4.0f});
    lights.push_back({float3{-0.24f, 1.97f, -0.22f}, float3{0.78f, 0.78f, 0.78f} / 4.0f});
    lights.push_back({float3{0.23f, 1.97f, -0.22f}, float3{0.78f, 0.78f, 0.78f} / 4.0f});
    lights.push_back({float3{0.23f, 1.97f, 0.16f}, float3{0.78f, 0.78f, 0.78f} / 4.0f});

    shadow_raytracer = std::make_shared<cg::renderer::raytracer<cg::vertex, cg::unsigned_color>>();
    shadow_raytracer->set_vertex_buffers(model->get_vertex_buffers());
    shadow_raytracer->set_index_buffers(model->get_index_buffers());
}

void cg::renderer::ray_tracing_renderer::destroy() {}

void cg::renderer::ray_tracing_renderer::update() {}

void cg::renderer::ray_tracing_renderer::render() {
    raytracer->clear_render_target({0, 0, 0});
    raytracer->build_acceleration_structure();
    raytracer->miss_shader = [](auto &r) {
        payload p{};
        p.color = {0.0f, 0.0f, 0.0f};
        return p;
    };
    shadow_raytracer->acceleration_structures = raytracer->acceleration_structures;
    shadow_raytracer->miss_shader = [](auto &r) {
        payload p{};
        p.t = -1.0f;
        return p;
    };
    shadow_raytracer->any_hit_shader = [](auto &ray, auto &payload, auto &triangle) {
        return payload;
    };
    std::random_device rd;
    std::mt19937 rng(rd());
    std::uniform_real_distribution<float> uniformRealDistribution(-1.0f, 1.0f);
    auto rt_depth = settings->raytracing_depth;
    raytracer->closest_hit_shader = [this, &uniformRealDistribution, &rng, rt_depth](auto &ray, auto &payload,
                                                                                     auto &triangle,
                                                                                     size_t depth) {
        float3 position = ray.position + ray.direction * payload.t;
        float3 normal = normalize(
                payload.bary.x * triangle.na +
                payload.bary.y * triangle.nb +
                payload.bary.z * triangle.nc
        );
        float3 result_color = triangle.emissive;

        float3 random_direction{uniformRealDistribution(rng), uniformRealDistribution(rng),
                                uniformRealDistribution(rng)};
        if (dot(normal, random_direction) < 0.0f) {
            random_direction = -random_direction;
        }
        cg::renderer::ray to_next_object(position, random_direction);
        auto payload_next = raytracer->trace_ray(to_next_object, depth);
        result_color += triangle.diffuse * payload_next.color.to_float3() *
                        std::max(dot(normal, to_next_object.direction), 0.0f);

        payload.color = cg::color::from_float3(result_color);
        return payload;
    };
    auto start = std::chrono::high_resolution_clock::now();
    raytracer->ray_generation(camera->get_position(), camera->get_direction(), camera->get_right(), camera->get_up(),
                              settings->raytracing_depth, settings->accumulation_num);
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "Render time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
              << "ms" << std::endl;
    cg::utils::save_resource(*render_target, settings->result_path);
}