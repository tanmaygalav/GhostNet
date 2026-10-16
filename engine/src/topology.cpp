#include "ghostnet/topology.hpp"
#include <sstream>

namespace ghostnet {

bool Topology::add_node(NodeConfig config) {
    if (nodes_.contains(config.id)) {
        return false;
    }
    auto id = config.id;
    nodes_[id] = std::make_unique<Node>(std::move(config));
    return true;
}

bool Topology::add_link(LinkConfig config) {
    if (links_.contains(config.id)) {
        return false;
    }
    LinkId lid = config.id;
    ComponentId src = config.source_id;
    links_[lid] = std::make_unique<Link>(std::move(config));
    outgoing_links_[src].push_back(lid);
    return true;
}

const Node* Topology::get_node(ComponentId id) const {
    auto it = nodes_.find(id);
    return (it != nodes_.end()) ? it->second.get() : nullptr;
}

Node* Topology::get_node_mut(ComponentId id) {
    auto it = nodes_.find(id);
    return (it != nodes_.end()) ? it->second.get() : nullptr;
}

const Link* Topology::get_link(LinkId id) const {
    auto it = links_.find(id);
    return (it != links_.end()) ? it->second.get() : nullptr;
}

const Link* Topology::find_link(ComponentId source, ComponentId target) const {
    auto it = outgoing_links_.find(source);
    if (it == outgoing_links_.end()) {
        return nullptr;
    }
    for (LinkId lid : it->second) {
        const auto* link = get_link(lid);
        if (link && link->target_id() == target) {
            return link;
        }
    }
    return nullptr;
}

std::vector<const Link*> Topology::get_outgoing_links(ComponentId source_id) const {
    std::vector<const Link*> result;
    auto it = outgoing_links_.find(source_id);
    if (it != outgoing_links_.end()) {
        result.reserve(it->second.size());
        for (LinkId lid : it->second) {
            if (const auto* l = get_link(lid)) {
                result.push_back(l);
            }
        }
    }
    return result;
}

ValidationResult Topology::validate() const {
    ValidationResult res;

    if (nodes_.empty()) {
        res.valid = false;
        res.errors.push_back("Topology contains no nodes.");
    }

    for (const auto& [lid, link] : links_) {
        if (!nodes_.contains(link->source_id())) {
            res.valid = false;
            std::ostringstream ss;
            ss << "Link " << lid << " references missing source node " << link->source_id() << ".";
            res.errors.push_back(ss.str());
        }
        if (!nodes_.contains(link->target_id())) {
            res.valid = false;
            std::ostringstream ss;
            ss << "Link " << lid << " references missing target node " << link->target_id() << ".";
            res.errors.push_back(ss.str());
        }
        if (link->source_id() == link->target_id()) {
            res.valid = false;
            std::ostringstream ss;
            ss << "Link " << lid << " is a self-loop (source == target).";
            res.errors.push_back(ss.str());
        }
        if (link->loss_probability() < 0.0 || link->loss_probability() > 1.0) {
            res.valid = false;
            std::ostringstream ss;
            ss << "Link " << lid << " loss probability must be in range [0.0, 1.0].";
            res.errors.push_back(ss.str());
        }
    }

    return res;
}

void Topology::clear() {
    nodes_.clear();
    links_.clear();
    outgoing_links_.clear();
}

} // namespace ghostnet