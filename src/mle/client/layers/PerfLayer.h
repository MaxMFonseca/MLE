#pragma once

#include "mle/client/Layer.h"
#include "mle/core/PerfTracker.h"
#include "mle/ui/UI.h"
#include "mle/window/UserInputManager.h"

namespace mle::client {
class PerfLayer : public mle::client::Layer {
  public:
    MLE_NO_COPY_MOVE(PerfLayer)

    PerfLayer() = default;
    ~PerfLayer() override = default;

    void init() override;
    void shutdown() override;

    void update() override;
    ImageRef render() override;
    void renderTo(ImageRef target) override;

  private:
    void toggle();
    struct ParsedSample {
        u64 last_sec_ns{0};
        f64 per_call_us{0.0};
        u64 calls_last_sec{0};
    };
    using ParsedSamples = std::map<std::string, std::map<std::string, ParsedSample>>;
    [[nodiscard]] ParsedSamples parseNewSamples();

  private:
    UI ui_;

    bool enabled_ = false;

    std::mutex new_samples_mutex_;
    PerfTracker::Samples new_samples_;

    PerfNewSamplesListenerHnd perf_listener_;

    KeyListenerHnd toggle_key_listener_;
};
}  // namespace mle::client
