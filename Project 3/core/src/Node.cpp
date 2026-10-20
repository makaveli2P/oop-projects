#include "strset/Node.h"

#include <atomic>
#include <utility>

namespace strset {

namespace {

std::atomic<NodeId> next_id{1};
std::atomic<std::int64_t> live_nodes{0};

}  // namespace

Node::Counter::Counter() noexcept { live_nodes.fetch_add(1, std::memory_order_relaxed); }
Node::Counter::~Counter() { live_nodes.fetch_sub(1, std::memory_order_relaxed); }

Node::Node(String value) : value_(std::move(value)), id_(next_id.fetch_add(1, std::memory_order_relaxed)) {}

std::int64_t Node::live_count() noexcept { return live_nodes.load(std::memory_order_relaxed); }

}  // namespace strset
