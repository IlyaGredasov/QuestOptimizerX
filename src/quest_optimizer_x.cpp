#include <quest_optimizer_x.hpp>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <numeric>
#include <queue>

int remain_quests(const std::vector<QuestLine> &quest_lines) {
	return std::accumulate(
			quest_lines.begin(),
			quest_lines.end(),
			0,
			[](const int acc, const QuestLine &line) {
				return acc + static_cast<int>(line.remaining());
			});
}

std::unordered_map<int, Path> QuestOptimizer::dijkstra_from(const int start) const {
	std::unordered_map<int, Path> best_paths;
	std::priority_queue<std::pair<double, std::vector<int>>,
						std::vector<std::pair<double, std::vector<int>>>,
						std::greater<>> pq;

	pq.emplace(0.0, std::vector{start});

	while (!pq.empty()) {
		const auto [length, path] = pq.top();
		pq.pop();
		int current = path.back();

		if (best_paths.contains(current) && best_paths[current].length <= length)
			continue;

		best_paths[current] = Path(path, length);

		for (int i = 0; i < graph_data.vertex_count; ++i) {
			if (const double w = graph_data.adj_list[current][i];
				std::isfinite(w)) {
				auto next_path = path;
				next_path.push_back(i);
				pq.emplace(length + w, next_path);
			}
		}
	}
	return best_paths;
}

void logger_thread_func(const std::atomic<unsigned> &found_best_paths, const std::atomic<unsigned> &minimum_quest_count,
						const std::atomic<bool> &stop_event, const std::set<PathState> &queue,
						std::mutex &queue_mutex, const float interval_seconds) {
	while (!stop_event.load(std::memory_order_relaxed)) {
		std::this_thread::sleep_for(std::chrono::duration<double>(interval_seconds));
		size_t queue_size = 0; {
			std::lock_guard lock(queue_mutex);
			queue_size = queue.size();
		}
		std::cout << "[Logger Thread] "
				<< "found_best_paths: " << found_best_paths.load(std::memory_order_relaxed)
				<< " minimum_quest_count: " << minimum_quest_count.load(std::memory_order_relaxed)
				<< " queue_size: " << queue_size
				<< "\n";
	}
}

void QuestOptimizer::update_state(PathState &current_state, bool &local_found) {
	current_state.path.vertexes.emplace_back(current_state.current_index);
	if (minimum_quest_count.load(std::memory_order_relaxed) == 0) {
		minimum_quest_count.store(remain_quests(graph_data.quest_lines), std::memory_order_relaxed);
	}
	if (remain_quests(current_state.quests) <= std::max(minimum_quest_count.load(std::memory_order_relaxed), 1u) *
		error_afford) {
		cv_queue.notify_all();
		for (auto it = current_state.quests.begin(); it != current_state.quests.end();) {
			if (!it->empty() && it->front() == current_state.current_index)
				it->pop_front();
			if (it->empty())
				it = current_state.quests.erase(it);
			else
				++it;
		}
		if (current_state.quests.empty()) {
			{
				std::lock_guard lock(queue_mutex);
				if (auto &current_best_path = best_path_for_start[current_state.path.vertexes[0]];
					current_best_path.vertexes.empty() || current_best_path.length > current_state.path.length)
					current_best_path = current_state.path;
				found_best_paths.fetch_add(1, std::memory_order_relaxed);
			}
			minimum_quest_count.store(remain_quests(graph_data.quest_lines), std::memory_order_relaxed);
			local_found = true;
		} else {
			for (int i = 0; i < graph_data.vertex_count; ++i) {
				if (const double edge_weight = graph_data.adj_list[current_state.current_index][i];
					std::isfinite(edge_weight)) {
					auto new_state = current_state;
					new_state.current_index = i;
					new_state.path.length += edge_weight;
					std::lock_guard lock(queue_mutex);
					if (queue.size() == max_queue_size) {
						const auto worst_it = std::prev(queue.end());
						const int rq_new = remain_quests(new_state.quests);
						const int rq_worst = remain_quests(worst_it->quests);
						if (rq_new < rq_worst ||
							(rq_new == rq_worst && new_state.path.length < worst_it->path.length)) {
							queue.erase(worst_it);
							queue.insert(new_state);
							if (sleeping_threads > 0)
								cv_queue.notify_one();
						}
					} else {
						queue.insert(new_state);
						if (sleeping_threads > 0 || queue.empty())
							cv_queue.notify_one();
					}
				}
			}
		}
	}
}

void QuestOptimizer::update_state_fast_travel(PathState &current_state, bool &local_found) {
	current_state.path.vertexes.emplace_back(current_state.current_index);
	if (minimum_quest_count.load(std::memory_order_relaxed) == 0) {
		minimum_quest_count.store(remain_quests(graph_data.quest_lines), std::memory_order_relaxed);
	}
	if (remain_quests(current_state.quests) <= std::max(minimum_quest_count.load(std::memory_order_relaxed), 1u) *
		error_afford) {
		cv_queue.notify_all();
		for (auto it = current_state.quests.begin(); it != current_state.quests.end();) {
			if (!it->empty() && it->front() == current_state.current_index)
				it->pop_front();
			if (it->empty())
				it = current_state.quests.erase(it);
			else
				++it;
		}
		if (current_state.quests.empty()) {
			{
				std::lock_guard lock(queue_mutex);
				if (auto &current_best_path = best_path_for_start[current_state.path.vertexes[0]];
					current_best_path.vertexes.empty() || current_best_path.length > current_state.path.length)
					current_best_path = current_state.path;
				found_best_paths.fetch_add(1, std::memory_order_relaxed);
			}
			minimum_quest_count.store(remain_quests(graph_data.quest_lines), std::memory_order_relaxed);
			local_found = true;
		} else {
			for (const auto &quest_line: current_state.quests) {
				auto new_state = current_state;
				new_state.current_index = quest_line.front();
				new_state.path.length += 1;
				std::lock_guard lock(queue_mutex);
				queue.insert(new_state);
			}
		}
	}
}


void QuestOptimizer::optimize_cycle() {
	bool local_found = false;
	const bool use_fast_travel = graph_data.fast_travel;
	while (!stop_event.load(std::memory_order_relaxed)) {
		PathState current_state; {
			std::unique_lock lock(queue_mutex);
			++sleeping_threads;
			cv_queue.wait(lock, [this] {
				return stop_event.load(std::memory_order_relaxed) || !queue.empty();
			});
			if (sleeping_threads == num_threads && queue.empty()) {
				stop_event.store(true, std::memory_order_relaxed);
				cv_queue.notify_all();
			}
			--sleeping_threads;
			if (stop_event.load(std::memory_order_relaxed)) {
				return;
			}
			if (queue.empty()) {
				continue;
			}
			const auto it = queue.begin();
			current_state = *it;
			queue.erase(it);
		}

		if (use_fast_travel) {
			update_state_fast_travel(current_state, local_found);
		} else {
			update_state(current_state, local_found);
		}

		if (!local_found && found_best_paths.load(std::memory_order_relaxed) < depth_of_search) {
			const unsigned remaining = remain_quests(current_state.quests);
			const unsigned prev_min = minimum_quest_count.load(std::memory_order_relaxed);
			minimum_quest_count.store(std::min(prev_min, remaining), std::memory_order_relaxed);
		}
		if (found_best_paths.load(std::memory_order_relaxed) >= depth_of_search) {
			stop_event.store(true, std::memory_order_relaxed);
			cv_queue.notify_all();
		}
	}
}

void QuestOptimizer::optimize() {
	for (int i = 0; i < graph_data.vertex_count; ++i) {
		queue.insert(PathState(i, Path{{}, 0.0}, graph_data.quest_lines));
	}
	auto threads = std::vector<std::thread>{};
	threads.reserve(num_threads);
	std::thread logger_thread{};
	if (std::abs(log_interval_seconds) >= std::numeric_limits<float>::epsilon()) {
		logger_thread = std::thread(logger_thread_func,
									std::ref(found_best_paths),
									std::ref(minimum_quest_count),
									std::ref(stop_event),
									std::ref(queue),
									std::ref(queue_mutex),
									log_interval_seconds);
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
		const auto it = std::ranges::min_element(best_path_for_start,
												[](const auto &a, const auto &b) {
													if (!std::isfinite(a.second.length))
														return false;
													if (!std::isfinite(b.second.length))
														return true;
													return a.second.vertexes.size() < b.second.vertexes.size();
												});
		if (it != best_path_for_start.end() && std::isfinite(it->second.length)) {
			best_path = it->second;
		} else {
			std::cerr << "[ERROR] No valid path found in best_path_for_start.\n";
		}
	} else {
		std::cout << "Dijkstra optimization\n";
		const auto start_paths = dijkstra_from(graph_data.start_index);
		best_path = Path({}, std::numeric_limits<double>::infinity());
		for (const auto &[via_vertex, coverage_path]: best_path_for_start) {
			auto it = start_paths.find(via_vertex);
			if (it == start_paths.end())
				continue;

			const Path &to_via = it->second;

			if (const double total_length = to_via.length + coverage_path.length;
				total_length < best_path.length) {
				best_path = to_via;
				best_path += coverage_path;
			}
		}
	}
}

Path QuestOptimizer::get_best_path() const {
	return best_path;
}

bool print_quests_on_path(
		const Path &path,
		const std::vector<QuestLine> &quest_lines,
		const std::vector<std::string> &vertex_names,
		const bool use_vertex_names,
		const bool use_quest_names
		) {
	std::cout << path.length << "\n";

	std::vector<std::pair<int, QuestLine> > quest_lines_dict;
	for (size_t i = 0; i < quest_lines.size(); ++i)
		quest_lines_dict.emplace_back(i, quest_lines[i]);

	for (const int vertex_index: path.vertexes) {
		if (use_vertex_names && vertex_index < static_cast<int>(vertex_names.size()))
			std::cout << vertex_names[vertex_index] << ":";
		else
			std::cout << vertex_index << ":";

		for (auto it = quest_lines_dict.begin(); it != quest_lines_dict.end();) {
			auto &[idx, quest_line] = *it;
			while (!quest_line.empty() && quest_line.front() == vertex_index) {
				if (use_quest_names)
					std::cout << quest_line.name << ' ';
				else
					std::cout << idx << ' ';
				quest_line.pop_front();
			}
			if (quest_line.empty())
				it = quest_lines_dict.erase(it);
			else
				++it;
		}
		std::cout << "\n";
	}
	return quest_lines_dict.empty();
}
