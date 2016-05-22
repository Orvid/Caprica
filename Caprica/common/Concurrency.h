#pragma once

// The purpose of this is to allow easy switching out between the concurrent collections
// offered by Microsoft's Concurrency Runtime and those provided by Intel's Thread
// Building Blocks.

// Kill the internal checking in the split list in debug mode,
// otherwise it's absurdly slow with large maps.
#ifdef _DEBUG
#undef _DEBUG
#include <concurrent_unordered_map.h>
#include <concurrent_unordered_set.h>
#define _DEBUG 1
#else
#include <concurrent_unordered_map.h>
#include <concurrent_unordered_set.h>
#endif

#include <common/CaselessStringComparer.h>

namespace caprica {

template<typename K>
using caseless_concurrent_unordered_identifier_set = typename Concurrency::concurrent_unordered_set<K, CaselessIdentifierHasher, CaselessIdentifierEqual>;
template<typename K, typename V>
using caseless_concurrent_unordered_identifier_map = typename Concurrency::concurrent_unordered_map<K, V, CaselessIdentifierHasher, CaselessIdentifierEqual>;
template<typename K, typename V>
using caseless_concurrent_unordered_path_map = typename Concurrency::concurrent_unordered_map<K, V, CaselessPathHasher, CaselessPathEqual>;

}
