#define TINYOBJLOADER_IMPLEMENTATION

#include "model.h"

#include "utils/error_handler.h"

#include <linalg.h>
#include <iostream>


using namespace linalg::aliases;

namespace cg::world {
    model::model() {}

    model::~model() {}

    void model::load_obj(const std::filesystem::path &model_path) {
        tinyobj::ObjReader reader;
        tinyobj::ObjReaderConfig config;
        config.mtl_search_path = model_path.parent_path().string();
        config.triangulate = true;
        if (!reader.ParseFromFile(model_path.string(), config)) {
            if (!reader.Error().empty()) {
                THROW_ERROR(reader.Error());
            }
            std::cout << reader.Warning() << std::endl;
        }
        allocate_buffers(reader.GetShapes());
        fill_buffers(reader.GetShapes(), reader.GetAttrib(), reader.GetMaterials(), model_path.parent_path());
        // TODO Lab: 1.03 Using `tinyobjloader` implement `load_obj`, `allocate_buffers`, `compute_normal`, `fill_vertex_data`, `fill_buffers`, `get_vertex_buffers`, `get_index_buffers` methods of `cg::world::model` class
    }

    void model::allocate_buffers(const std::vector<tinyobj::shape_t> &shapes) {
        for (const auto &shape: shapes) {
            unsigned int vertex_buffer_size = 0;
            std::map<std::tuple<int, int, int>, unsigned int> index_map;
            auto &mesh = shape.mesh;
            for (const auto &ind: mesh.indices) {
                auto ind_tuple = std::make_tuple(ind.vertex_index, ind.normal_index, ind.texcoord_index);
                if (index_map.find(ind_tuple) == index_map.end()) {
                    index_map[ind_tuple] = vertex_buffer_size;
                    vertex_buffer_size++;
                }
            }
            vertex_buffers.push_back(std::make_shared<cg::resource<cg::vertex>>(vertex_buffer_size));
            index_buffers.push_back(std::make_shared<cg::resource<unsigned int>>(mesh.indices.size()));
        }
        textures.resize(shapes.size());
    }

    float3 model::compute_normal(const tinyobj::attrib_t &attrib, const tinyobj::mesh_t &mesh, size_t index_offset) {
        auto &a_ind = mesh.indices[index_offset];
        auto &b_ind = mesh.indices[index_offset + 1];
        auto &c_ind = mesh.indices[index_offset + 2];
        float3 a{
                attrib.vertices[3 * a_ind.vertex_index],
                attrib.vertices[3 * a_ind.vertex_index + 1],
                attrib.vertices[3 * a_ind.vertex_index + 2],
        };
        float3 b{
                attrib.vertices[3 * b_ind.vertex_index],
                attrib.vertices[3 * b_ind.vertex_index + 1],
                attrib.vertices[3 * b_ind.vertex_index + 2],
        };
        float3 c{
                attrib.vertices[3 * c_ind.vertex_index],
                attrib.vertices[3 * c_ind.vertex_index + 1],
                attrib.vertices[3 * c_ind.vertex_index + 2],
        };
        return normalize(cross(b - a, c - a));
    }

    void model::fill_vertex_data(cg::vertex &vertex, const tinyobj::attrib_t &attrib, const tinyobj::index_t idx,
                                 const float3 computed_normal, const tinyobj::material_t material) {
        vertex.x = attrib.vertices[3 * idx.vertex_index];
        vertex.y = attrib.vertices[3 * idx.vertex_index + 1];
        vertex.z = attrib.vertices[3 * idx.vertex_index + 2];

        if (idx.normal_index < 0) {
            vertex.nx = computed_normal.x;
            vertex.ny = computed_normal.y;
            vertex.nz = computed_normal.z;
        } else {
            vertex.nx = attrib.normals[3 * idx.normal_index];
            vertex.ny = attrib.normals[3 * idx.normal_index + 1];
            vertex.nz = attrib.normals[3 * idx.normal_index + 2];
        }
        if (idx.texcoord_index < 0) {
            vertex.u = 0;
            vertex.v = 0;
        } else {
            vertex.u = attrib.texcoords[2 * idx.texcoord_index];
            vertex.v = attrib.texcoords[2 * idx.texcoord_index + 1];
        }
        vertex.ambient_r = material.ambient[0];
        vertex.ambient_g = material.ambient[1];
        vertex.ambient_b = material.ambient[2];

        vertex.diffuse_r = material.diffuse[0];
        vertex.diffuse_g = material.diffuse[1];
        vertex.diffuse_b = material.diffuse[2];

        vertex.emissive_r = material.emission[0];
        vertex.emissive_g = material.emission[1];
        vertex.emissive_b = material.emission[2];
    }

    void model::fill_buffers(
            const std::vector<tinyobj::shape_t> &shapes,
            const tinyobj::attrib_t &attrib,
            const std::vector<tinyobj::material_t> &materials,
            const std::filesystem::path &base_folder
    ) {
        for (int i = 0; i < shapes.size(); ++i) {
            auto &mesh = shapes[i].mesh;
            auto &vertex_buffer = vertex_buffers[i];
            auto &index_buffer = index_buffers[i];
            unsigned int vertex_buffer_offset = 0;
            unsigned int index_buffer_offset = 0;
            std::map<std::tuple<int, int, int>, unsigned int> index_map;
            size_t index_offset = 0;
            for (int f = 0; f < mesh.num_face_vertices.size(); ++f) {
                auto fv = mesh.num_face_vertices[f];
                float3 normal;
                if (mesh.indices[index_offset].normal_index < 0) {
                    normal = compute_normal(attrib, mesh, index_offset);
                }
                for (int j = 0; j < fv; ++j) {
                    auto &idx = mesh.indices[index_offset + j];
                    auto idx_tuple = std::make_tuple(idx.vertex_index, idx.normal_index, idx.texcoord_index);
                    if (index_map.find(idx_tuple) == index_map.end()) {
                        auto &vertex = vertex_buffer->item(vertex_buffer_offset);
                        auto &material = materials[mesh.material_ids[f]];
                        fill_vertex_data(vertex, attrib, idx, normal, material);
                        index_map[idx_tuple] = vertex_buffer_offset;
                        vertex_buffer_offset++;
                    }
                    index_buffer->item(index_buffer_offset) = index_map[idx_tuple];
                    index_buffer_offset++;
                }
                index_offset += fv;
            }
            if (!materials[mesh.material_ids[0]].diffuse_texname.empty()) {
                textures[i] = base_folder / materials[mesh.material_ids[0]].diffuse_texname;
            }
        }
    }


    const std::vector<std::shared_ptr<resource<vertex>>> &model::get_vertex_buffers() const {
        return vertex_buffers;
    }

    const std::vector<std::shared_ptr<resource<unsigned int>>> &model::get_index_buffers() const {
        return index_buffers;
    }

    const std::vector<std::filesystem::path> &model::get_per_shape_texture_files() const {
        return textures;
    }


    const float4x4 model::get_world_matrix() const {
        return float4x4{
                {1, 0, 0, 0},
                {0, 1, 0, 0},
                {0, 0, 1, 0},
                {0, 0, 0, 1}};
    }

}
