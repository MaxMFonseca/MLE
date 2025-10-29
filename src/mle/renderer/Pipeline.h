#pragma once

#include "Shader.h"
#include "Types.h"
#include "mle/core/Assert.h"
#include "vulkan/vulkan.hpp"

namespace mle {
class Pipeline final {
  public:
    struct CreateInfo {
        Shader const* vertex_shader = nullptr;
        Shader const* fragment_shader = nullptr;
        Shader const* compute_shader = nullptr;
        vk::PrimitiveTopology topology = vk::PrimitiveTopology::eTriangleList;
        vk::PolygonMode polygon_mode = vk::PolygonMode::eFill;
        vk::CullModeFlags cull_mode = vk::CullModeFlagBits::eBack;
        vk::FrontFace front_face = vk::FrontFace::eCounterClockwise;
        std::span<const vk::Format> color_attachment_formats;
        std::span<const vk::PipelineColorBlendAttachmentState> blend_attachments;
        std::span<const vk::DynamicState> dynamic_states;
        bool depth = false;
        bool depth_write = true;
        bool depth_bias = false;
        u8 push_descriptor = max<u8>();

        std::vector<std::pair<u8, vk::DescriptorSetLayout>> external_descriptor_set_layouts;
    };

    using CI = CreateInfo;  ///< Alias for CreateInfo.
  public:
    MLE_NO_COPY_MOVE(Pipeline)

    ~Pipeline();

    void init(const CI& ci);

    [[nodiscard]] auto get() const { return o_; }
    [[nodiscard]] auto hasPushConstants() const { return pc_size_ > 0; }
    [[nodiscard]] auto getPushConstantSize() const { return pc_size_; }
    [[nodiscard]] auto getPushConstantFragOffset() const { return pc_frag_offset_; }
    [[nodiscard]] auto getPipelineLayout() const { return pipeline_layout_; }
    [[nodiscard]] auto hasInstance() const { return first_instance_binding_ != max<u8>(); }
    [[nodiscard]] auto getFirstInstanceBinding() const { return first_instance_binding_; }
    [[nodiscard]] auto isCompute() const { return compute_; }
    [[nodiscard]] const Shader::PushConstantField& getPushConstantField(std::string_view name) const;
    [[nodiscard]] const auto& getPushConstantFields() const { return pc_fields_; }

    template <usize Size>
    static constexpr std::array<vk::PipelineColorBlendAttachmentState, Size> makeDefaultBlendAttachments() {
        std::array<vk::PipelineColorBlendAttachmentState, Size> blend_attachments{};
        for (usize i = 0; i < Size; i++) {
            blend_attachments[i].blendEnable = VK_TRUE;
            blend_attachments[i].srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
            blend_attachments[i].dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
            blend_attachments[i].colorBlendOp = vk::BlendOp::eAdd;
            blend_attachments[i].srcAlphaBlendFactor = vk::BlendFactor::eSrcAlpha;
            blend_attachments[i].dstAlphaBlendFactor = vk::BlendFactor::eDstAlpha;
            blend_attachments[i].alphaBlendOp = vk::BlendOp::eMax;
            blend_attachments[i].colorWriteMask =
                vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
        }
        return blend_attachments;
    }
    static constexpr std::vector<vk::PipelineColorBlendAttachmentState> makeDefaultBlendAttachments(usize size) {
        std::vector<vk::PipelineColorBlendAttachmentState> blend_attachments{};
        blend_attachments.resize(size);
        for (usize i = 0; i < size; i++) {
            blend_attachments[i].blendEnable = VK_TRUE;
            blend_attachments[i].srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
            blend_attachments[i].dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
            blend_attachments[i].colorBlendOp = vk::BlendOp::eAdd;
            blend_attachments[i].srcAlphaBlendFactor = vk::BlendFactor::eSrcAlpha;
            blend_attachments[i].dstAlphaBlendFactor = vk::BlendFactor::eDstAlpha;
            blend_attachments[i].alphaBlendOp = vk::BlendOp::eMax;
            blend_attachments[i].colorWriteMask =
                vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
        }
        return blend_attachments;
    }

  private:
    template <typename Elem>
    void setPayload(vk::WriteDescriptorSet& w, const Elem* ptr) const {
        if constexpr (std::is_same_v<Elem, const vk::DescriptorBufferInfo>) {
            w.pBufferInfo = ptr;
        } else if constexpr (std::is_same_v<Elem, const vk::DescriptorImageInfo>) {
            w.pImageInfo = ptr;
        } else if constexpr (std::is_same_v<Elem, const vk::BufferView>) {
            w.pTexelBufferView = ptr;
        } else {
            MLE_UNREACHABLE_LOG("Unsupported descriptor info type for WriteDescriptorSet. {}", __PRETTY_FUNCTION__);
        }
    }

  public:
    template <typename... InfosPtr>
    [[nodiscard]] auto makeWrites(usize set, vk::DescriptorSet dst_set, const InfosPtr*... infos_ptrs) const {
        constexpr usize SIZE = sizeof...(InfosPtr);
        using Ret = std::array<vk::WriteDescriptorSet, SIZE>;

        using Tup = std::tuple<const InfosPtr*...>;
        const Tup tup{infos_ptrs...};

        const auto it = std::ranges::find_if(ds_infos_, [set](const auto& ds) { return ds.set == set; });
        if (it == ds_infos_.end()) {
            MLE_E("Descriptor set {} not found in pipeline descriptor sets.", set);
            return Ret{};
        }
        const Shader::DescriptorSet& dsi = *it;
        MLE_ASSERT_LOG(dsi.bindings.size() >= SIZE, "Requested {} writes but set {} has only {} bindings.", SIZE, set, dsi.bindings.size());

        std::array<vk::WriteDescriptorSet, SIZE> writes{};

        auto build_at = [&](auto Iconst) {
            constexpr usize I = decltype(Iconst)::value;

            const auto& b = dsi.bindings[I];

            vk::WriteDescriptorSet w{};
            w.dstSet = dst_set;
            w.dstBinding = b.binding;
            w.dstArrayElement = 0;
            w.descriptorType = b.descriptorType;
            w.descriptorCount = b.descriptorCount;

            using Ptr = std::tuple_element_t<I, Tup>;
            using Elem = std::remove_pointer_t<std::remove_cv_t<Ptr>>;
            const Ptr info_ptr = std::get<I>(tup);

            setPayload<Elem>(w, info_ptr);

            writes[I] = w;
        };

        [&]<std::size_t... Is>(std::index_sequence<Is...>) { (build_at(std::integral_constant<std::size_t, Is>{}), ...); }(std::make_index_sequence<SIZE>{});

        return writes;
    }

  private:
    friend PipelineCache;
    Pipeline() = default;
    PipelineHnd static createHnd(const CI& ci);

    void createGraphicsPipeline(const CI& ci);
    void createComputePipeline(const CI& ci);

    void createPipelineLayout(const CI& ci);

  private:
    vk::Pipeline o_ = nullptr;
    vk::PipelineLayout pipeline_layout_ = nullptr;

    std::vector<Shader::PushConstantField> pc_fields_;  ///< Push constant fields.
    u8 pc_size_ = 0;                                    ///< Size of the push constant block.
    u8 pc_frag_offset_ = max<u8>();                     ///< Offset of fragment shader push constants.
    u8 first_instance_binding_ = max<u8>();             ///< First instance attribute binding location.
    bool compute_ = false;                              ///< Whether this is a compute pipeline.

    std::vector<Shader::DescriptorSet> ds_infos_;      ///< Descriptor set infos for this pipeline.
    std::vector<vk::DescriptorSetLayout> owned_dsls_;  ///< Descriptor set layouts owned by this pipeline.
};
}  // namespace mle
