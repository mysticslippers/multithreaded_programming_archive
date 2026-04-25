#pragma once

#include <pthread.h>

#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace sanitizer {

using MutexId = std::uintptr_t;

struct DeadlockReport {
    bool has_cycle = false;
    std::vector<MutexId> cycle;
    MutexId new_from = 0;
    MutexId new_to = 0;
};

class LockGraph {
public:
    DeadlockReport add_edge(MutexId from, MutexId to);

    [[nodiscard]] bool has_edge(MutexId from, MutexId to) const;

    [[nodiscard]] std::size_t vertex_count() const;
    [[nodiscard]] std::size_t edge_count() const;

private:
    std::unordered_map<MutexId, std::unordered_set<MutexId>> edges_;

    [[nodiscard]] bool find_path(
        MutexId current,
        MutexId target,
        std::unordered_set<MutexId>& visited,
        std::vector<MutexId>& path
    ) const;
};

[[nodiscard]] MutexId mutex_to_id(const pthread_mutex_t* mutex);
std::string format_cycle(const std::vector<MutexId>& cycle);

}