#pragma once

#include <optional>
#include <vector>
#include <unordered_map>
#include <cstdio>

namespace jlc {

// Returns the (start, end) of a generic container, as a pair.
// TODO: Does not work, would be cool to implement in the future
template <class Container> inline constexpr auto range(Container c) {
    return std::pair(std::begin(c), std::end(c));
}

// Returns the value& specified by the key as an optional
// On success: opt<Val&>
// On failure: opt<null>

namespace map {
template <class Key, class Val>
inline auto getValue(const Key& key, std::unordered_map<Key, Val>& map) {
    auto search = map.find(key);
    bool success = search != map.end();
    return success ? std::optional<std::reference_wrapper<Val>>{search->second}
                   : std::nullopt;
}
}

// Tries to read the file with the given name. If fileName is empty,
// start reading from std in. Throws on failure, so guarantees to return a valid
// file-pointer.
inline FILE* readFileOrInput(const char* fileName) {
    FILE* input;
    if (fileName) {
        input = fopen(fileName, "r");
        if (!input)
            throw std::exception();
    } else {
        input = stdin;
    }
    return input;
}

} // namespace jlc
