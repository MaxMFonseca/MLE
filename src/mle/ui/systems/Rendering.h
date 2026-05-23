#pragma once

#include "../Types.h"
#include "mle/renderer/Buffer.h"
#include "mle/renderer/Image.h"
#include "mle/renderer/RenderingThread.h"
#include "mle/renderer/Types.h"
#include "mle/ui/components/Base.h"
#include "mle/ui/components/Bounds.h"
#include "mle/ui/components/Renderable.h"
#include "mle/ui/components/Shader.h"
#include "mle/ui/renderable/RenderableI.h"
#include "mle/utils/containers/AtomicTripleBuffer.h"

// FIXME: rework the border mechanic
// use 9slice rendering. ?

namespace mle::ui::system {
class Rendering {
  public:
    struct Packet {
        u64 id = 0;

        struct Node {
            entt::entity e_id = entt::null;

            std::vector<Node> children;

            comp::Bounds bounds{};
            comp::Border border{};
            struct {
                Color lt = Color::ZERO;
                Color rt = Color::ZERO;
                Color lb = Color::ZERO;
                Color rb = Color::ZERO;
            } bg;

            int layer = 0;

            f32 scale_factor = 1.0F;

            std::shared_ptr<ShaderPacketI> shader_packet;
            std::shared_ptr<RenderablePacketI> renderable_packet;
            bool renderable_dedicate_render_target = false;
            bool shader_before_children = false;
            bool shader_dedicate_render_target = false;
            Color shader_clear_color = Color::ZERO;
        };

        Node root{};
    };

    struct RenderingContext {
        RenderingThread thread;
        Rectf parent_viewport{};
        Recti parent_scissor{};
        f32 parent_scale = 1.0F;

        // TODO: should I instead of rendering immediate add everything to this context and bind all pipelines once ?
        // YEAH sure
    };

  public:
    explicit Rendering(UI& ui);
    ~Rendering() = default;

    MLE_NO_COPY_MOVE(Rendering);

    void update();
    [[nodiscard]] ImageRef render();
    [[nodiscard]] Expected<ImageRef> getRenderImage(entt::entity entity);
    void clear();

  private:
    Expected<Packet::Node> createPacketNode(u8 atomic_buffer_id, entt::entity entity, usize depth = 0);

    [[nodiscard]] ImageRef getDedicatedImageForEntity(entt::entity entity, vec2u extent);
    [[nodiscard]] ImageRef getExistingDedicatedImageForEntity(entt::entity entity, vec2u extent);
    [[nodiscard]] ImageRef getImageForNode(const Packet::Node& node);

    void renderNode(const Rendering::Packet::Node& node, RenderingContext& ctx);
    std::unique_ptr<RenderingContext> renderCreateNodeNewContext(const Rendering::Packet::Node& node);
    void renderNodeBorder(const Rendering::Packet::Node& node, Recti ppx, RenderingContext& ctx);
    void renderNodeBackground(const Rendering::Packet::Node& node, Recti ppx, RenderingContext& ctx);
    void renderNodeRenderable(const Rendering::Packet::Node& node, RenderingContext& ctx);
    void renderNodeChildren(const Rendering::Packet::Node& node, RenderingContext& ctx);
    void renderNodeShader(const Rendering::Packet::Node& node, RenderingContext& ctx);

    void updateRenderable();

    void renderableCreated(entt::registry& registry, entt::entity entity);
    void renderableDestroyed(entt::registry& registry, entt::entity entity);
    void shaderCreated(entt::registry& registry, entt::entity entity);
    void shaderDestroyed(entt::registry& registry, entt::entity entity);

  private:
    [[maybe_unused]] UI& ui_;
    Lua lua_;
    AtomicTripleBuffer<Packet> atomic_data_;
    u64 next_packet_id_ = 0;

    std::map<entt::entity, std::array<ImageHnd, 2>> dedicated_images_;

    ImageRef last_rendered_image_ = nullptr;

    const Pipeline* background_pipeline_ = nullptr;
    const Pipeline* border_pipeline_ = nullptr;
};
}  // namespace mle::ui::system
