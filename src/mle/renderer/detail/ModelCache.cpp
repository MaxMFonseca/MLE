#include "ModelCache.h"

#include <vulkan/vulkan.hpp>
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
void loadVertices(const tinygltf::Model& model, const tinygltf::Primitive& primitive, std::vector<VertexPCN>& vertices) {
    auto read_attribute = [&](const std::string& attr_name) -> const float* {
        if (!primitive.attributes.contains(attr_name)) {
            return nullptr;
        }

        const auto& accessor = model.accessors[primitive.attributes.at(attr_name)];
        const auto& view = model.bufferViews[accessor.bufferView];
        const auto& buffer = model.buffers[view.buffer];

        return reinterpret_cast<const float*>(buffer.data.data() + view.byteOffset + accessor.byteOffset);  // NOLINT
    };

    const float* pos_data = read_attribute("POSITION");
    const float* norm_data = read_attribute("NORMAL");
    const float* color_data = read_attribute("COLOR_0");

    const auto& accessor = model.accessors[primitive.attributes.at("POSITION")];
    const size_t count = accessor.count;
    vertices.reserve(count);

    for (size_t i = 0; i < count; ++i) {
        VertexPCN v{};
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

        vertices.push_back(v);
    }
}

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
}  // namespace

void ModelCache::init() {  // NOLINT static
    MLE_D("Initializing model cache");

    MLE_NOOP;
}

void ModelCache::reset() {
    MLE_D("Resetting model cache");

    models_.clear();
}

Expected<Model> ModelCache::get(const std::string& name) {
    auto it = models_.find(name);
    if (it == models_.end()) {
        auto lm_result = loadModel(name);
        if (lm_result == Result::OK) {
            return std::unexpected(Result::NOT_READY);
        }
        return std::unexpected(lm_result);
    }
    auto& model_data = it->second;
    if (model_data.state == ModelData::State::OUT) {
        auto lm_result = loadModel(name);
        if (lm_result == Result::OK) {
            return std::unexpected(Result::NOT_READY);
        }
        return std::unexpected(lm_result);
    }
    if (model_data.state == ModelData::State::UPLOADING) {
        return std::unexpected(Result::NOT_READY);
    }
    MLE_ASSERT(model_data.index_count > 0);
    return Model{.vertex_buffer = model_data.vertex_buffer.get(), .index_buffer = model_data.index_buffer.get(), .index_count = model_data.index_count};
}

Result ModelCache::loadModel(const std::string& name, std::function<void(Model)>&& callback) {
    MLE_D("Loading model: {}", name);

    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string err, warn;

    std::string file_name{"res/models/" + name + ".glb"};

    // TODO: try to recover
    if (!loader.LoadBinaryFromFile(&model, &err, &warn, file_name)) {
        MLE_E("Failed to load model '{}': {}", name, err);
        MLE_TODO;
        return Result::FAILED_TO_OPEN;
    }
    if (!warn.empty()) {
        MLE_E("Warning while loading model '{}': {}", name, warn);
        MLE_TODO;
        return Result::FAILED_TO_OPEN;
    }
    if (!err.empty()) {
        MLE_E("Error while loading model '{}': {}", name, err);
        MLE_TODO;
        return Result::FAILED_TO_OPEN;
    }

    MLE_C("Mesh count: {}", model.meshes.size());

    // TODO: MAYBE this
    MLE_ASSERT_LOG(model.meshes.size() == 1, "Model '{}' should have exactly one mesh, found: {}", name, model.meshes.size());
    MLE_ASSERT_LOG(model.meshes[0].primitives.size() == 1, "Model '{}' should have exactly one primitive, found: {}", name, model.meshes[0].primitives.size());

    PCNMeshData vertex_data;
    loadVertices(model, model.meshes[0].primitives[0], vertex_data.vertices);
    loadIndices(model, model.meshes[0].primitives[0], vertex_data.indices);

    return add(name, vertex_data, std::move(callback));
}

Result ModelCache::add(const std::string& name, const PCNMeshData& vertex_data, std::function<void(Model)>&& callback) {
    const auto& vertices = vertex_data.vertices;
    const auto& indices = vertex_data.indices;

    auto up_it = std::ranges::find_if(uploading_, [name](const auto& u) { return u.name == name; });
    if (up_it == uploading_.end()) {
        uploading_.emplace_back(name);
        up_it = std::prev(uploading_.end());
    }
    auto& uploading_data = *up_it;

    if (callback) {
        uploading_data.callbacks.push_back(std::move(callback));
    }

    auto& model_data = models_[name];
    if (model_data.state == ModelData::State::UPLOADING) {
        MLE_UNREACHABLE_LOG("Model '{}' is already loading", name);
        return Result::OK;
    }
    MLE_ASSERT_LOG(model_data.state != ModelData::State::OK, "Somehow load model was called for model '{}' that is already loaded.", name);

    model_data.index_count = static_cast<int>(indices.size());

    Buffer::CI vertex_ci{};
    vertex_ci.size = vertices.size() * sizeof(VertexPCN);
    vertex_ci.usage = vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst;
    vertex_ci.allocation_type = Buffer::CI::AllocationType::GPU_ONLY;
    model_data.vertex_buffer = Buffer::createHnd(vertex_ci);

    Buffer::CI index_ci{};
    index_ci.size = indices.size() * sizeof(u32);
    index_ci.usage = vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst;
    index_ci.allocation_type = Buffer::CI::AllocationType::GPU_ONLY;
    model_data.index_buffer = Buffer::createHnd(index_ci);

    auto tcmd = renderer::getOTSCmd(CmdType::TRANSFER);
    auto gcmd = renderer::getOTSCmd(CmdType::GRAPHICS);

    uploading_data.semaphore = unwrap(detail::getDevice().createSemaphore({}));
    uploading_data.v_staging = model_data.vertex_buffer->updateStaged(tcmd, vertices.data());
    uploading_data.i_staging = model_data.index_buffer->updateStaged(tcmd, indices.data());

    vk::BufferMemoryBarrier2 v_mem_barrier = {};
    v_mem_barrier.srcStageMask = vk::PipelineStageFlagBits2::eTransfer;
    v_mem_barrier.srcAccessMask = vk::AccessFlagBits2::eTransferWrite;
    v_mem_barrier.dstStageMask = vk::PipelineStageFlagBits2::eAllCommands;
    v_mem_barrier.dstAccessMask = {};
    v_mem_barrier.srcQueueFamilyIndex = detail::getVk().getQueueIndex(CmdType::TRANSFER);
    v_mem_barrier.dstQueueFamilyIndex = detail::getVk().getQueueIndex(CmdType::GRAPHICS);
    v_mem_barrier.buffer = model_data.vertex_buffer->getVkHnd();
    v_mem_barrier.size = vk::WholeSize;

    vk::DependencyInfo v_dep_info = {};
    v_dep_info.setBufferMemoryBarriers(v_mem_barrier);

    tcmd.pipelineBarrier2(v_dep_info);
    gcmd.pipelineBarrier2(v_dep_info);

    // auto i_mem_barrier = v_mem_barrier;
    // i_mem_barrier.buffer = model_data.index_buffer->getVkHnd();
    //
    // vk::DependencyInfo i_dep_info = {};
    // i_dep_info.setBufferMemoryBarriers(i_mem_barrier);
    //
    // tcmd.pipelineBarrier2(i_dep_info);
    // gcmd.pipelineBarrier2(i_dep_info);

    vk::SemaphoreSubmitInfo semaphore_info = {};
    semaphore_info.setSemaphore(uploading_data.semaphore);
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

    return Result::OK;
}

void ModelCache::finishedUpload(const std::string& name) {
    MLE_D("Finished uploading model: {}", name);

    auto it = std::ranges::find_if(uploading_, [name](const auto& u) { return u.name == name; });
    MLE_ASSERT_LOG(it != uploading_.end(), "Model '{}' not found in uploading models", name);

    auto& model_data = models_[name];
    model_data.state = ModelData::State::OK;

    detail::getDevice().destroy(it->semaphore);

    for (auto& callback : it->callbacks) {
        callback(Model{.vertex_buffer = model_data.vertex_buffer.get(), .index_buffer = model_data.index_buffer.get(), .index_count = model_data.index_count});
    }

    uploading_.erase(it);
}

void ModelCache::addCallbackToUploaded(const std::string& name, std::function<void(Model)>&& callback) {
    MLE_D("Adding callback for model upload: {}", name);

    auto it = std::ranges::find_if(uploading_, [name](const auto& u) { return u.name == name; });
    if (it != uploading_.end()) {
        it->callbacks.push_back(std::move(callback));
        return;
    }

    auto& up_data = uploading_.emplace_back();
    up_data.name = name;
    up_data.callbacks.push_back(std::move(callback));
}
}  // namespace mle::renderer::detail
