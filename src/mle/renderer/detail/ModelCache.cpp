#include "ModelCache.h"

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_structs.hpp>

#include "mle/common/Assert.h"
#include "mle/common/Result.h"
#include "mle/core/Core.h"
#include "mle/renderer/Buffer.h"
#include "mle/renderer/Renderer.h"
#include "mle/renderer/Types.h"
#include "mle/renderer/Utils.h"
#include "mle/renderer/detail/VkContext.h"

namespace mle::renderer::detail {
namespace {
void loadIndices(const tinygltf::Model& model, const tinygltf::Primitive& primitive, std::vector<u32>& indices) {
    const auto& accessor = model.accessors[primitive.indices];
    const auto& view = model.bufferViews[accessor.bufferView];
    const auto& buffer = model.buffers[view.buffer];

    const u8* data = buffer.data.data() + view.byteOffset + accessor.byteOffset;  // NOLINT

    for (size_t i = 0; i < accessor.count; ++i) {
        switch (accessor.componentType) {
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
                indices.push_back(as<u32>(data[i]));  // NOLINT
                break;
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                indices.push_back(as<u32>(rAs<const u16*>(data)[i]));
                break;
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
                indices.push_back(rAs<const u32*>(data)[i]);
                break;
            default:
                MLE_UNREACHABLE_LOG("Unsupported index type");
        }
    }
}

UploadVoxMeshData loadVoxMesh(const tinygltf::Model& model, const tinygltf::Mesh& mesh) {
    MLE_C("Mesh name: {}", mesh.name);

    UploadVoxMeshData upload_data;

    auto split_name = split(mesh.name, ';');
    if (split_name.size() > 1) {
        for (usize i = 1; i < split_name.size(); ++i) {
            char type = split_name[i][0];
            f32 val = std::stof(split_name[i].substr(1));
            switch (type) {
                case 'm': {
                    upload_data.metalness = val;
                } break;
                case 'e': {
                    upload_data.emissive = val;
                } break;
                case 'r': {
                    upload_data.roughness = val;
                } break;
                default: {
                    MLE_W("Unknown mesh property type: '{}'", type);
                } break;
            }
        }
    }

    MLE_ASSERT_LOG(mesh.primitives.size() == 1, "Vox mesh should have exactly one primitive, found: {}", mesh.primitives.size());

    const auto& primitive = mesh.primitives[0];

    auto read_attribute = [&](const std::string& attr_name) -> const float* {
        if (!primitive.attributes.contains(attr_name)) {
            return nullptr;
        }

        const auto& accessor = model.accessors[primitive.attributes.at(attr_name)];
        const auto& view = model.bufferViews[accessor.bufferView];
        const auto& buffer = model.buffers[view.buffer];

        return rAs<const float*>(buffer.data.data() + view.byteOffset + accessor.byteOffset);  // NOLINT
    };

    const float* pos_data = read_attribute("POSITION");
    const float* norm_data = read_attribute("NORMAL");
    const float* color_data = read_attribute("COLOR_0");

    const auto& accessor = model.accessors[primitive.attributes.at("POSITION")];
    const usize count = accessor.count;
    upload_data.aabb_min = {accessor.minValues[0], accessor.minValues[1], accessor.minValues[2]};
    upload_data.aabb_max = {accessor.maxValues[0], accessor.maxValues[1], accessor.maxValues[2]};

    upload_data.vertices.reserve(count);

    for (size_t i = 0; i < count; ++i) {
        PCNVertex v{};
        if (pos_data) {
            v.pos.x = pos_data[i * 3 + 0];  // NOLINT
            v.pos.y = pos_data[i * 3 + 1];  // NOLINT
            v.pos.z = pos_data[i * 3 + 2];  // NOLINT
        }
        if (norm_data) {
            v.normal.x = norm_data[i * 3 + 0];  // NOLINT
            v.normal.y = norm_data[i * 3 + 1];  // NOLINT
            v.normal.z = norm_data[i * 3 + 2];  // NOLINT
        }
        if (color_data) {
            v.color.x = color_data[i * 3 + 0];  // NOLINT
            v.color.y = color_data[i * 3 + 1];  // NOLINT
            v.color.z = color_data[i * 3 + 2];  // NOLINT
        }

        upload_data.vertices.push_back(v);
    }

    loadIndices(model, primitive, upload_data.indices);

    return upload_data;
}
}  // namespace

void ModelCache::init() {  // NOLINT static
    MLE_D("Initializing model cache");

    MLE_NOOP;
}

void ModelCache::reset() {
    MLE_D("Resetting model cache");

    models_.clear();
}

ModelRef ModelCache::get(const std::string& name) {
    auto it = models_.find(name);
    if (it == models_.end()) {
        return loadModel(name);
    }
    auto& model_data = it->second;
    if (model_data->state == UploadState::OUT) {
        return loadModel(name);
    }

    return it->second.get();
}

ModelRef ModelCache::loadModel(const std::string& name) {
    auto it = models_.find(name);
    if (it != models_.end() && it->second->state != UploadState::OUT) {
        return it->second.get();
    }

    MLE_D("Loading model: {}", name);

    // TODO: try to recover

    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string err, warn;
    std::string file_name{"res/models/" + name + ".glb"};
    if (!loader.LoadBinaryFromFile(&model, &err, &warn, file_name) || !warn.empty() || !err.empty()) {
        core::unrecoverable("Failed to load model '{}' warning: '{}' error: '{}'", name, warn, err);
    }

    MLE_C("Mesh count: {}", model.meshes.size());

    UploadModelData upload_model_data;

    for (auto& mesh : model.meshes) {
        MLE_C("Mesh name: {}", mesh.name);
        if (mesh.primitives.empty()) {
            MLE_W("Mesh '{}' has no primitives, skipping", mesh.name);
            continue;
        }

        if (mesh.primitives[0].mode != TINYGLTF_MODE_TRIANGLES) {
            MLE_W("Mesh '{}' is not a triangle mesh, skipping", mesh.name);
            continue;
        }

        upload_model_data.meshes.emplace_back(loadVoxMesh(model, mesh));
    }

    return add(name, upload_model_data);
}

ModelRef ModelCache::add(const std::string& name, const UploadModelData& model_data) {
    auto it = models_.find(name);
    if (it == models_.end()) {
        it = models_.emplace(name, std::make_unique<Model>()).first;
    }
    if (it->second->state != UploadState::OUT) {
        return it->second.get();
    }

    auto up_it = std::ranges::find_if(uploading_, [name](const auto& u) { return u.name == name; });
    MLE_ASSERT_LOG(up_it == uploading_.end(), "Model '{}' is already being uploaded", name);

    auto& model = it->second;
    model->state = UploadState::UPLOADING;

    auto& uploading = uploading_.emplace_back();
    uploading.name = name;
    uploading.meshes.reserve(model_data.meshes.size());
    model->meshes.reserve(model_data.meshes.size());

    auto tcmd = renderer::getOTSCmd(CmdType::TRANSFER);
    auto gcmd = renderer::getOTSCmd(CmdType::GRAPHICS);

    uploading.semaphore = unwrap(detail::getDevice().createSemaphore({}));

    vec3f aabb_min{max<f32>()};
    vec3f aabb_max{min<f32>()};

    for (const auto& mesh_data : model_data.meshes) {
        auto& model_mesh = model->meshes.emplace_back();
        auto& uploading_mesh = uploading.meshes.emplace_back();

        model_mesh.type = Model::Mesh::Type::VOX;

        MLE_ASSERT_LOG(std::holds_alternative<UploadVoxMeshData>(mesh_data), "Only UploadVoxMeshData is supported for now");
        const auto& vox_mesh_data = std::get<UploadVoxMeshData>(mesh_data);

        aabb_min = glm::min(aabb_min, vox_mesh_data.aabb_min);
        aabb_max = glm::max(aabb_max, vox_mesh_data.aabb_max);

        model_mesh.metalness = vox_mesh_data.metalness;
        model_mesh.roughness = vox_mesh_data.roughness;
        model_mesh.emissive = vox_mesh_data.emissive;

        const auto& vertices = vox_mesh_data.vertices;
        const auto& indices = vox_mesh_data.indices;

        model_mesh.index_count = as<int>(indices.size());

        Buffer::CI vertex_ci{};
        vertex_ci.size = vertices.size() * sizeof(PCNVertex);
        vertex_ci.usage = vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst;
        vertex_ci.allocation_type = Buffer::CI::AllocationType::GPU_ONLY;
        model_mesh.vertex_buffer = Buffer::createHnd(vertex_ci);

        Buffer::CI index_ci{};
        index_ci.size = indices.size() * sizeof(u32);
        index_ci.usage = vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst;
        index_ci.allocation_type = Buffer::CI::AllocationType::GPU_ONLY;
        model_mesh.index_buffer = Buffer::createHnd(index_ci);

        uploading_mesh.v_staging = model_mesh.vertex_buffer->updateStaged(tcmd, vertices.data());
        uploading_mesh.i_staging = model_mesh.index_buffer->updateStaged(tcmd, indices.data());

        vk::BufferMemoryBarrier2 v_mem_barrier = {};
        v_mem_barrier.srcStageMask = vk::PipelineStageFlagBits2::eTransfer;
        v_mem_barrier.srcAccessMask = vk::AccessFlagBits2::eTransferWrite;
        v_mem_barrier.dstStageMask = vk::PipelineStageFlagBits2::eAllCommands;
        v_mem_barrier.dstAccessMask = vk::AccessFlagBits2::eVertexAttributeRead;
        v_mem_barrier.srcQueueFamilyIndex = detail::getVk().getQueueIndex(CmdType::TRANSFER);
        v_mem_barrier.dstQueueFamilyIndex = detail::getVk().getQueueIndex(CmdType::GRAPHICS);
        v_mem_barrier.buffer = model_mesh.vertex_buffer->getVkHnd();
        v_mem_barrier.size = vk::WholeSize;

        vk::DependencyInfo v_dep_info = {};
        v_dep_info.setBufferMemoryBarriers(v_mem_barrier);

        tcmd.pipelineBarrier2(v_dep_info);
        gcmd.pipelineBarrier2(v_dep_info);

        auto i_mem_barrier = v_mem_barrier;
        i_mem_barrier.dstAccessMask = vk::AccessFlagBits2::eIndexRead;
        i_mem_barrier.buffer = model_mesh.index_buffer->getVkHnd();

        vk::DependencyInfo i_dep_info = {};
        i_dep_info.setBufferMemoryBarriers(i_mem_barrier);

        tcmd.pipelineBarrier2(i_dep_info);
        gcmd.pipelineBarrier2(i_dep_info);
    }

    vec3f box_size = aabb_max - aabb_min;
    MLE_ASSERT_LOG(box_size.length() > 0 && box_size.length() <= 1000, "Box size is invalid: {}", box_size);
    model->aabb = Boxf{aabb_min, box_size};

    vk::SemaphoreSubmitInfo semaphore_info = {};
    semaphore_info.setSemaphore(uploading.semaphore);
    semaphore_info.setStageMask(vk::PipelineStageFlagBits2::eTransfer);
    vk::CommandBufferSubmitInfo cmd_info = {};
    cmd_info.setCommandBuffer(tcmd);
    vk::SubmitInfo2 submit_info = {};
    submit_info.setCommandBufferInfos(cmd_info);
    submit_info.setSignalSemaphoreInfos(semaphore_info);

    renderer::submitOTSAsync(CmdType::TRANSFER, submit_info);

    cmd_info.setCommandBuffer(gcmd);
    submit_info.setSignalSemaphoreInfos({});
    submit_info.setWaitSemaphoreInfos(semaphore_info);

    renderer::submitOTSAsync(CmdType::GRAPHICS, submit_info, [this, name]() { finishedUpload(name); });

    return it->second.get();
}

void ModelCache::finishedUpload(const std::string& name) {
    MLE_D("Finished uploading model: {}", name);

    auto it = std::ranges::find_if(uploading_, [name](const auto& u) { return u.name == name; });
    MLE_ASSERT_LOG(it != uploading_.end(), "Model '{}' not found in uploading models", name);

    auto& model_data = models_[name];
    model_data->state = UploadState::OK;

    detail::getDevice().destroy(it->semaphore);

    uploading_.erase(it);
}
}  // namespace mle::renderer::detail
