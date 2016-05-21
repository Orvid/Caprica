#pragma once

// The purpose of this is to allow easy switching out between the concurrent collections
// offered by Microsoft's Concurrency Runtime and those provided by Intel's Thread
// Building Blocks.

#include <concurrent_unordered_map.h>

#include <common/CaselessStringComparer.h>

namespace caprica {

template<typename K, typename V>
using concurrent_caseless_unordered_path_map = typename Concurrency::concurrent_unordered_map<K, V, CaselessPathHasher, CaselessPathEqual>;

}
