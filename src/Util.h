#pragma once
#include <optional>
#include <vector>

// Returns the (start, end) of a generic container, as a pair.
// TODO: Does not work, would be cool to implement in the future
template <class Container> constexpr auto range(Container c) {
    return std::pair(std::begin(c), std::end(c));
}

template <class Key, class Val>
auto findIn(const Key& key, std::unordered_map<Key, Val>& map) {
    auto search = map.find(key);
    bool success = search != map.end();
    return success ? std::optional<std::reference_wrapper<Val>>{search->second}
                   : std::nullopt;
}