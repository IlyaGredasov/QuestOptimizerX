#pragma once

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <limits>
#include <memory_resource>
#include <mutex>
#include <ranges>
#include <set>
#include <thread>
#include <unordered_map>
#include <vector>

#include "parser.hpp"

struct Path {
    std::vector<int> vertexes;
    double length = 0.0;

    Path& operator+=(const Path& other) {
        vertexes.insert(vertexes.end(), other.vertexes.begin(), other.vertexes.end());
        length += other.length;
        return *this;
    }
};

int remain_quests(std::vector<QuestLine>::const_iterator first, std::vector<QuestLine>::const_iterator last);

bool print_quests_on_path(
    const Path& path,
    const std::vector<QuestLine>& quest_lines,
    const std::vector<std::string>& vertex_names,
    bool use_vertex_names,
    bool use_quest_names
);

struct PathState {
    int current_index;
    Path path;
    std::vector<size_t> quest_positions;
    int remaining_quest_count;

    bool operator<(const PathState& other) const {
        if (remaining_quest_count != other.remaining_quest_count) {
            return remaining_quest_count < other.remaining_quest_count;
        }
        if (path.length != other.path.length)
            return path.length < other.path.length;
        return current_index < other.current_index;
    }
};

class QuestOptimizer final {
public:
    explicit QuestOptimizer(
        const GraphData& graph_data,
        const unsigned num_threads = std::thread::hardware_concurrency(),
        const unsigned max_queue_size = 100000,
        const double error_afford = 1.05,
        const unsigned depth_of_search = 1,
        const float log_interval_seconds = 1.0
    )
        : graph_data(graph_data),
          num_threads(num_threads),
          max_queue_size(max_queue_size),
          error_afford(error_afford),
          depth_of_search(depth_of_search),
          log_interval_seconds(log_interval_seconds),
          total_quest_count(remain_quests(graph_data.quest_lines.begin(), graph_data.quest_lines.end())),
          minimum_quest_count(total_quest_count) {
        std::ranges::for_each(std::views::iota(0, graph_data.vertex_count), [&](const int i) {
            best_path_for_start[i] = Path({}, std::numeric_limits<double>::infinity());
        });
    };

    void optimize();

    Path get_best_path() const;

private:
    const GraphData& graph_data;
    const unsigned num_threads;
    const unsigned max_queue_size;
    const double error_afford;
    const unsigned depth_of_search;
    const float log_interval_seconds;
    int total_quest_count;

    std::atomic<unsigned> found_best_paths = 0;
    std::atomic<unsigned> minimum_quest_count;
    std::unordered_map<int, Path> best_path_for_start;
    Path best_path = {.vertexes = {}, .length = std::numeric_limits<double>::infinity()};

    std::pmr::unsynchronized_pool_resource queue_pool{};
    std::pmr::set<PathState> queue{&queue_pool};
    std::condition_variable cv_queue;
    unsigned sleeping_threads = 0;
    std::mutex queue_mutex;
    std::atomic<bool> stop_event{false};

    std::unordered_map<int, Path> dijkstra_from(int start) const;

    bool update_state(PathState& current_state);
    bool update_state_fast_travel(PathState& current_state);

    void optimize_cycle();
};
