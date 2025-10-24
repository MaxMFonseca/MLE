#pragma once

#include "../Types.h"
#include "mle/renderer/Buffer.h"
#include "mle/renderer/Image.h"
#include "mle/renderer/RenderingThread.h"
#include "mle/renderer/Types.h"
#include "mle/ui/components/Base.h"
#include "mle/ui/components/Bounds.h"
#include "mle/ui/components/Shader.h"
#include "mle/ui/renderable/RenderableI.h"
#include "mle/utils/AtmoicTripleBuffer.h"

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
            Color bg{};
            comp::Shader shader{};

            bool renderable = true;
        };

        Node root{};
    };

    struct RenderingContext {
        RenderingThread rendering_thread;
        Recti parent_viewport{};
        Recti current_scissor{};
    };

  public:
    explicit Rendering(UI& ui);
    ~Rendering() = default;

    MLE_NO_COPY_MOVE(Rendering);

    void update();
    [[nodiscard]] ImageRef render();

  private:
    Packet createPacket();
    Packet::Node createPacketNode(entt::entity entity, usize depth = 0);

    [[nodiscard]] ImageRef getImageForEntity(const Packet::Node& node);

    void renderNode(const Rendering::Packet::Node& node, RenderingContext& ctx);

    void updateRenderable();

  private:
    [[maybe_unused]] UI& ui_;
    AtomicTripleBuffer<Packet> atomic_data_;
    u64 next_packet_id_ = 0;

    std::map<entt::entity, ImageHnd> root_images_;

    std::unordered_map<entt::entity, AtomicTripleBuffer<std::unique_ptr<RenderablePacketI>>> renderable_packets_;

    ImageRef last_rendered_image_ = nullptr;

    const Pipeline* background_pipeline_ = nullptr;
    const Pipeline* border_pipeline_ = nullptr;

    entt::storage_for_t<entt::reactive>& renderable_created_storage_;
    entt::storage_for_t<entt::reactive>& renderable_destroyed_storage_;
};
}  // namespace mle::ui::system
