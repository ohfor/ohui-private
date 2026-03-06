#pragma once

#include "ohui/dsl/ComponentHandler.h"
#include "ohui/message/MessageStream.h"

namespace ohui::dsl {

// --- MessageFeed: stacked message list with per-message fade ---
class MessageFeedHandler : public IComponentHandler {
public:
    void Emit(const ComponentContext& ctx, DrawCallList& output) override;

    void SetMessageStream(message::MessageStream* stream) { m_stream = stream; }

private:
    message::MessageStream* m_stream{nullptr};
};

}  // namespace ohui::dsl
