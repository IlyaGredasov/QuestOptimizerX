#include "quest_optimizer_x.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <numeric>
#include <queue>
#include <utility>

void atomic_fetch_min(std::atomic<unsigned>& value, const unsigned candidate) {
    auto current = value.load(std::memory_order_relaxed);
    while (candidate < current &&
           !value.compare_exchange_weak(current, candidate, std::memory_order_relaxed, std::memory_order_relaxed)) {}
}

int remain_quests(const std::vector<QuestLine>::const_iterator first,
    const std::vector<QuestLine>::const_iterator last) {
    return std::accumulate(first, last, 0,
        [](const int acc, const QuestLine& line) { return acc + static_cast<int>(line.vertexes.size()); });
}

std::unordered_map<int, Path> QuestOptimizer::dijkstra_from(const int start) const {
    std::unordered_map<int, Path> best_paths;
    std::priority_queue<std::pair<double, std::vector<int>>, std::vector<std::pair<double, std::vector<int>>>,
        std::greater<>>
        pq;
    pq.emplace(0.0, std::vector{start});

    while (!pq.empty()) {
        const auto [length, path] = pq.top();
        pq.pop();
        const int current = path.back();

        if (best_paths.contains(current) && best_paths[current].length <= length)
            continue;

        best_paths[current] = Path(path, length);

        for (const auto& edge : graph_data.adj_list[current]) {
            auto next_path = path;
            next_path.emplace_back(edge.to);
            pq.emplace(length + edge.weight, std::move(next_path));
        }
    }
    return best_paths;
}

void logger_thread_func(const std::atomic<unsigned>& found_best_paths, const std::atomic<unsigned>& minimum_quest_count,
    const std::atomic<bool>& stop_event, const std::pmr::set<PathState>& queue, std::mutex& queue_mutex,
    const float interval_seconds) {
    while (!stop_event.load(std::memory_order::acquire)) {
        std::this_thread::sleep_for(std::chrono::duration<double>(interval_seconds));
        size_t queue_size = 0;
        {
            const std::scoped_lock lock(queue_mutex);
            queue_size = queue.size();
        }
        std::cout << "[Logger Thread] "
                  << "found_best_paths: " << found_best_paths.load(std::memory_order::acquire)
                  << " minimum_quest_count: " << minimum_quest_count.load(std::memory_order::acquire)
                  << " queue_size: " << queue_size << std::endl;
    }
}

bool QuestOptimizer::update_state(PathState& current_state) {
    bool local_found = false;
    current_state.path.vertexes.emplace_back(current_state.current_index);
    if (minimum_quest_count.load(std::memory_order::acquire) == 0) {
        minimum_quest_count.store(total_quest_count, std::memory_order::release);
    }
    if (current_state.remaining_quest_count <=
        std::max(minimum_quest_count.load(std::memory_order::acquire), 1u) * error_afford) {
        cv_queue.notify_all();
        for (size_t quest_id = 0; quest_id < graph_data.quest_lines.size(); ++quest_id) {
            const auto& quest_line = graph_data.quest_lines[quest_id];
            auto& position = current_state.quest_positions[quest_id];
            if (position < quest_line.vertexes.size() && quest_line.vertexes[position] == current_state.current_index) {
                ++position;
                --current_state.remaining_quest_count;
            }
        }
        if (current_state.remaining_quest_count == 0) {
            {
                const std::scoped_lock lock(queue_mutex);
                if (auto& current_best_path = best_path_for_start[current_state.path.vertexes[0]];
                    current_best_path.vertexes.empty() || current_best_path.length > current_state.path.length)
                    current_best_path = current_state.path;
                found_best_paths.fetch_add(1, std::memory_order::release);
            }
            minimum_quest_count.store(total_quest_count, std::memory_order::release);
            local_found = true;
        } else {
            for (const auto& edge : graph_data.adj_list[current_state.current_index]) {
                auto new_state = current_state;
                new_state.current_index = edge.to;
                new_state.path.length += edge.weight;
                const std::scoped_lock lock(queue_mutex);
                if (queue.size() == max_queue_size) {
                    const auto worst_it = std::prev(queue.end());
                    const int rq_new = new_state.remaining_quest_count;
                    const int rq_worst = worst_it->remaining_quest_count;
                    if (rq_new < rq_worst || (rq_new == rq_worst && new_state.path.length < worst_it->path.length)) {
                        queue.erase(worst_it);
                        queue.insert(std::move(new_state));
                        if (sleeping_threads > 0)
                            cv_queue.notify_one();
                    }
                } else {
                    queue.insert(std::move(new_state));
                    if (sleeping_threads > 0 || queue.empty())
                        cv_queue.notify_one();
                }
            }
        }
    }
    return local_found;
}

bool QuestOptimizer::update_state_fast_travel(PathState& current_state) {
    bool local_found = false;
    current_state.path.vertexes.emplace_back(current_state.current_index);
    if (minimum_quest_count.load(std::memory_order::acquire) == 0) {
        minimum_quest_count.store(total_quest_count, std::memory_order::release);
    }
    if (current_state.remaining_quest_count <=
        std::max(minimum_quest_count.load(std::memory_order::acquire), 1u) * error_afford) {
        cv_queue.notify_all();
        for (size_t quest_id = 0; quest_id < graph_data.quest_lines.size(); ++quest_id) {
            const auto& quest_line = graph_data.quest_lines[quest_id];
            auto& position = current_state.quest_positions[quest_id];
            if (position < quest_line.vertexes.size() && quest_line.vertexes[position] == current_state.current_index) {
                ++position;
                --current_state.remaining_quest_count;
            }
        }
        if (current_state.remaining_quest_count == 0) {
            {
                const std::scoped_lock lock(queue_mutex);
                if (auto& current_best_path = best_path_for_start[current_state.path.vertexes[0]];
                    current_best_path.vertexes.empty() || current_best_path.length > current_state.path.length)
                    current_best_path = current_state.path;
                found_best_paths.fetch_add(1, std::memory_order::release);
            }
            minimum_quest_count.store(total_quest_count, std::memory_order::release);
            local_found = true;
        } else {
            for (size_t quest_id = 0; quest_id < graph_data.quest_lines.size(); ++quest_id) {
                const auto& quest_line = graph_data.quest_lines[quest_id];
                const auto position = current_state.quest_positions[quest_id];
                if (position >= quest_line.vertexes.size())
                    continue;
                auto new_state = current_state;
                new_state.current_index = quest_line.vertexes[position];
                new_state.path.length += 1;
                const std::scoped_lock lock(queue_mutex);
                queue.insert(std::move(new_state));
            }
        }
    }
    return local_found;
}

void QuestOptimizer::optimize_cycle() {
    bool local_found = false;
    const bool use_fast_travel = graph_data.fast_travel;
    while (!stop_event.load(std::memory_order::acquire)) {
        PathState current_state;
        {
            std::unique_lock lock(queue_mutex);
            ++sleeping_threads;
            cv_queue.wait(lock, [this] { return stop_event.load(std::memory_order::acquire) || !queue.empty(); });
            if (sleeping_threads == num_threads && queue.empty()) {
                stop_event.store(true, std::memory_order::release);
                cv_queue.notify_all();
            }
            --sleeping_threads;
            if (stop_event.load(std::memory_order::acquire)) {
                return;
            }
            if (queue.empty()) {
                continue;
            }
            const auto it = queue.begin();
            current_state = *it;
            queue.erase(it);
        }

        local_found = use_fast_travel ? update_state_fast_travel(current_state) : update_state(current_state);

        if (!local_found && found_best_paths.load(std::memory_order::acquire) < depth_of_search) {
            const unsigned remaining = current_state.remaining_quest_count;
            atomic_fetch_min(minimum_quest_count, remaining);
        }
        if (found_best_paths.load(std::memory_order::acquire) >= depth_of_search) {
            stop_event.store(true, std::memory_order::release);
            cv_queue.notify_all();
        }
    }
}

void QuestOptimizer::optimize() {
    const std::vector<size_t> initial_quest_positions(graph_data.quest_lines.size(), 0);
    for (int i = 0; i < graph_data.vertex_count; ++i) {
        queue.insert(PathState(i, Path{{}, 0.0}, initial_quest_positions, total_quest_count));
    }
    auto threads = std::vector<std::thread>{};
    threads.reserve(num_threads);
    std::thread logger_thread{};
    if (std::abs(log_interval_seconds) >= std::numeric_limits<float>::epsilon()) {
        logger_thread = std::thread(logger_thread_func, std::ref(found_best_paths), std::ref(minimum_quest_count),
            std::ref(stop_event), std::ref(queue), std::ref(queue_mutex), log_interval_seconds);
    }
    for (unsigned i = 0; i < num_threads; ++i) {
        threads.emplace_back(&QuestOptimizer::optimize_cycle, this);
    }
    for (unsigned i = 0; i < num_threads; ++i) {
        threads[i].join();
    }
    if (std::abs(log_interval_seconds) >= std::numeric_limits<float>::epsilon()) {
        logger_thread.join();
    }
    if (graph_data.start_index == -1) {
        const auto it = std::ranges::min_element(best_path_for_start, [](const auto& a, const auto& b) {
            if (!std::isfinite(a.second.length))
                return false;
            if (!std::isfinite(b.second.length))
                return true;
            return a.second.vertexes.size() < b.second.vertexes.size();
        });
        if (it != best_path_for_start.end() && std::isfinite(it->second.length)) {
            best_path = it->second;
        } else {
            std::cerr << "[ERROR] No valid path found in best_path_for_start." << std::endl;
        }
    } else {
        std::cout << "Dijkstra optimization" << std::endl;
        const auto start_paths = dijkstra_from(graph_data.start_index);
        best_path = Path({}, std::numeric_limits<double>::infinity());
        for (const auto& [via_vertex, coverage_path] : best_path_for_start) {
            auto it = start_paths.find(via_vertex);
            if (it == start_paths.end())
                continue;

            const Path& to_via = it->second;

            if (const double total_length = to_via.length + coverage_path.length; total_length < best_path.length) {
                best_path = to_via;
                best_path += coverage_path;
            }
        }
    }
}

Path QuestOptimizer::get_best_path() const { return best_path; }

bool print_quests_on_path(const Path& path, const std::vector<QuestLine>& quest_lines,
    const std::vector<std::string>& vertex_names, const bool use_vertex_names, const bool use_quest_names) {
    std::cout << path.length << std::endl;
    std::vector<size_t> quest_positions(quest_lines.size(), 0);
    size_t completed_quests =
        std::ranges::count_if(quest_lines, [](const QuestLine& quest_line) { return quest_line.vertexes.empty(); });

    for (const int vertex_index : path.vertexes) {
        if (use_vertex_names && std::cmp_less(vertex_index, vertex_names.size()))
            std::cout << vertex_names[vertex_index] << ":";
        else
            std::cout << vertex_index << ":";

        for (size_t quest_id = 0; quest_id < quest_lines.size(); ++quest_id) {
            const auto& quest_line = quest_lines[quest_id];
            auto& position = quest_positions[quest_id];
            if (position == quest_line.vertexes.size())
                continue;
            while (position < quest_line.vertexes.size() && quest_line.vertexes[position] == vertex_index) {
                if (use_quest_names)
                    std::cout << quest_line.name << ' ';
                else
                    std::cout << quest_id << ' ';
                ++position;
            }
            if (position == quest_line.vertexes.size())
                ++completed_quests;
        }
        std::cout << std::endl;
    }
    return completed_quests == quest_lines.size();
}
