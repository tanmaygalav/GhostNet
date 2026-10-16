#pragma once

#include "ghostnet/node.hpp"
#include "ghostnet/link.hpp"
#include <unordered_map>
#include <vector>
#include <memory>
#include <string>

namespace ghostnet {

struct ValidationResult {
    bool valid{true};
    std::vector<std::string> errors{};
};

class Topology {
public:
    Topology() = default;

    bool add_node(NodeConfig config);
    bool add_link(LinkConfig config);

    [[nodiscard]] const Node* get_node(ComponentId id) const;
    [[nodiscard]] Node* get_node_mut(ComponentId id);

    [[nodiscard]] const Link* get_link(LinkId id) const;
    [[nodiscard]] const Link* find_link(ComponentId source, ComponentId target) const;

    [[nodiscard]] std::vector<const Link*> get_outgoing_links(ComponentId source_id) const;

    [[nodiscard]] size_t node_count() const noexcept { return nodes_.size(); }
    [[nodiscard]] size_t link_count() const noexcept { return links_.size(); }

    // FR-007 Validation: Checks endpoint existence, parameter limits, self-loops, duplicate edges
    [[nodiscard]] ValidationResult validate() const;

    void clear();

private:
    std::unordered_map<ComponentId, std::unique_ptr<Node>> nodes_;
    std::unordered_map<LinkId, std::unique_ptr<Link>> links_;
    std::unordered_map<ComponentId, std::vector<LinkId>> outgoing_links_;
};

} // namespace ghostnet