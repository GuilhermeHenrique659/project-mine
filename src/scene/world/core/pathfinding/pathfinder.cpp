#include "scene/world/core/pathfinding/pathfinder.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <queue>
#include <unordered_map>

namespace world {

namespace {

constexpr float Diagonal = 1.41421356f;

std::uint64_t key_of(TileCoord tile) {
    return (static_cast<std::uint64_t>(static_cast<std::uint32_t>(tile.x)) << 32) |
           static_cast<std::uint32_t>(tile.y);
}

// Octile distance: exact on an empty 8-neighbour grid, so A* stays optimal.
float heuristic(TileCoord from, TileCoord to) {
    const int dx = std::abs(from.x - to.x);
    const int dy = std::abs(from.y - to.y);
    return static_cast<float>(std::max(dx, dy)) +
           (Diagonal - 1.0f) * static_cast<float>(std::min(dx, dy));
}

struct Node {
    float cost = 0.0f;
    TileCoord parent{};
    bool closed = false;
};

struct Open {
    float priority;
    TileCoord tile;
    bool operator>(const Open& other) const { return priority > other.priority; }
};

std::vector<TileCoord> rebuild(const std::unordered_map<std::uint64_t, Node>& nodes,
                               TileCoord start, TileCoord goal) {
    std::vector<TileCoord> path{goal};
    TileCoord at = goal;
    while (!(at == start)) {
        at = nodes.at(key_of(at)).parent;
        path.push_back(at);
    }
    std::reverse(path.begin(), path.end());
    return path;
}

} // namespace

std::optional<std::vector<TileCoord>> find_path(TileCoord start, TileCoord goal,
                                                const BlockedFn& is_blocked, int max_nodes) {
    const auto walkable = [&](TileCoord tile) {
        return tile == start || tile == goal || !is_blocked(tile);
    };

    std::unordered_map<std::uint64_t, Node> nodes;
    std::priority_queue<Open, std::vector<Open>, std::greater<>> open;
    nodes[key_of(start)] = Node{0.0f, start, false};
    open.push({heuristic(start, goal), start});

    int expanded = 0;
    while (!open.empty()) {
        const TileCoord at = open.top().tile;
        open.pop();
        Node& current = nodes[key_of(at)];
        if (current.closed) {
            continue;
        }
        if (at == goal) {
            return rebuild(nodes, start, goal);
        }
        if (++expanded > max_nodes) {
            return std::nullopt;
        }
        current.closed = true;
        const float cost = current.cost;

        for (int dx = -1; dx <= 1; ++dx) {
            for (int dy = -1; dy <= 1; ++dy) {
                if (dx == 0 && dy == 0) {
                    continue;
                }
                const TileCoord next{at.x + dx, at.y + dy};
                if (!walkable(next)) {
                    continue;
                }
                const bool diagonal = dx != 0 && dy != 0;
                if (diagonal && (!walkable({at.x + dx, at.y}) || !walkable({at.x, at.y + dy}))) {
                    continue;
                }
                const float next_cost = cost + (diagonal ? Diagonal : 1.0f);
                const auto found = nodes.find(key_of(next));
                if (found != nodes.end() && (found->second.closed || found->second.cost <= next_cost)) {
                    continue;
                }
                nodes[key_of(next)] = Node{next_cost, at, false};
                open.push({next_cost + heuristic(next, goal), next});
            }
        }
    }
    return std::nullopt;
}

} // namespace world
