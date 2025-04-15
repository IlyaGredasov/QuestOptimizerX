#include <quest_optimizer_x.hpp>
#include <bits/random.h>
#include <random>
#include <iostream>

int remain_quests(const std::vector<QuestLine> &quest_lines) {
	return std::accumulate(
		quest_lines.begin(),
		quest_lines.end(),
		0,
		[](const int acc, const QuestLine &line) {
			return acc + static_cast<int>(line.vertexes.size());
		});
}

std::vector<std::vector<Path> > QuestOptimizer::get_floyd_paths() const {
	if (fast_travel) {
		std::vector ways(
			graph_data.adj_list.size(),
			std::vector<Path>(graph_data.adj_list.size())
		);
		for (int i = 0; i < static_cast<int>(graph_data.adj_list.size()); ++i) {
			for (int j = 0; j < static_cast<int>(graph_data.adj_list.size()); ++j) {
				if (i!=j)
					ways[i][j] = Path({i, j}, 1.0);
			}
		}
		return ways;
	}
	auto path_matrix = graph_data.adj_list;
	std::vector next_node(path_matrix.size(), std::vector<int>(path_matrix.size()));
	for (size_t i = 0; i < path_matrix.size(); ++i)
		for (size_t j = 0; j < path_matrix.size(); ++j)
			next_node[i][j] = static_cast<int>(j);
	for (size_t i2 = 0; i2 < path_matrix.size(); ++i2) {
		for (size_t i1 = 0; i1 < path_matrix.size(); ++i1) {
			for (size_t i3 = 0; i3 < path_matrix.size(); ++i3) {
				if (path_matrix[i1][i2] + path_matrix[i2][i3] < path_matrix[i1][i3]) {
					path_matrix[i1][i3] = path_matrix[i1][i2] + path_matrix[i2][i3];
					next_node[i1][i3] = next_node[i1][i2];
				}
			}
		}
	}
	auto ways = std::vector(
		path_matrix.size(),
		std::vector(path_matrix.size(), Path{{}, std::numeric_limits<double>::infinity()})
	);
	for (size_t i1 = 0; i1 < path_matrix.size(); ++i1) {
		for (size_t i2 = 0; i2 < path_matrix.size(); ++i2) {
			if (path_matrix[i1][i2] != std::numeric_limits<double>::infinity()) {
				auto way = Path({}, 0);
				size_t current = i1;
				while (current != i2) {
					way += Path({static_cast<int>(current)}, graph_data.adj_list[current][next_node[current][i2]]);
					current = next_node[current][i2];
				}
				way.vertexes.emplace_back(current);
				ways[i1][i2] = way;
			}
		}
	}
	return ways;
}

void QuestOptimizer::optimize_cycle() {
	std::mt19937 gen(std::random_device{}());
	bool local_found = false;
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
			std::uniform_int_distribution dist(0, static_cast<int>((queue.size() - 1) * queue_narrowness));
			const auto it = queue.find_by_order(dist(gen));
			current_state = *it;
			queue.erase(it);
		}

		current_state.path.vertexes.emplace_back(current_state.current_index);
		if (minimum_quest_count.load(std::memory_order_relaxed) == 0) {
			minimum_quest_count.store(remain_quests(graph_data.quest_lines), std::memory_order_relaxed);
		}
		if (remain_quests(current_state.quests) <= std::max(minimum_quest_count.load(std::memory_order_relaxed), 1) *
			error_afford) {
			std::cout << "found_best_paths: " << found_best_paths.load(std::memory_order_relaxed) <<
					" minimum_quest_count: "
					<< minimum_quest_count.load(std::memory_order_relaxed) << " queue_size: " << queue.size() << ENDL;
			cv_queue.notify_all();
			for (auto it = current_state.quests.begin(); it != current_state.quests.end();) {
				if (!it->vertexes.empty() && it->vertexes.front() == current_state.current_index)
					it->vertexes.pop_front();
				if (it->vertexes.empty()) it = current_state.quests.erase(it);
				else ++it;
			}
			if (current_state.quests.empty()) {
				{
					std::lock_guard lock(queue_mutex);
					if (auto &current_best_path = best_path_for_start[current_state.path.vertexes[0]];
						current_best_path.vertexes.empty() || current_best_path.length > current_state.path.length)
						current_best_path = current_state.path;
					found_best_paths.fetch_add(1, std::memory_order_relaxed);
				}
				for (const auto t: current_state.path.vertexes) {
					std::cout << t << ' ';
				}
				std::cout << ENDL;
				minimum_quest_count.store(remain_quests(graph_data.quest_lines), std::memory_order_relaxed);
				local_found = true;
			} else if (fast_travel) {
				for (const auto &quest_line: current_state.quests) {
					auto new_state = current_state;
					new_state.current_index = quest_line.vertexes.front();
					new_state.path.length += 1;

					std::lock_guard lock(queue_mutex);
					queue.insert(new_state);
				}
			} else {
				for (int i = 0; i < graph_data.vertex_count; ++i) {
					if (const double edge_weight = graph_data.adj_list[current_state.current_index][i];
						std::isfinite(edge_weight)) {
						auto new_state = current_state;
						new_state.current_index = i;
						new_state.path.length += edge_weight;
						std::lock_guard lock(queue_mutex);
						if (queue.size() == max_queue_size) {
							std::uniform_int_distribution dist(
								0, static_cast<int>((queue.size() - 1) * queue_narrowness));
							if (const auto it = queue.find_by_order(dist(gen));
								remain_quests(it->quests) > remain_quests(new_state.quests)) {
								queue.erase(it);
								queue.insert(new_state);
							}
						} else {
							queue.insert(new_state);
						}
					}
				}
			}
		}
		if (!local_found && found_best_paths.load(std::memory_order_relaxed) < deep_of_search) {
			const int remaining = remain_quests(current_state.quests);
			const int prev_min = minimum_quest_count.load(std::memory_order_relaxed);
			minimum_quest_count.store(std::min(prev_min, remaining), std::memory_order_relaxed);
		}
		if (found_best_paths.load(std::memory_order_relaxed) >= deep_of_search) {
			stop_event.store(true, std::memory_order_relaxed);
			cv_queue.notify_all();
		}
	}
}

void QuestOptimizer::optimize_floyd() {
	std::cout << "Floyd optimization" << ENDL;
	const auto floyd_ways = get_floyd_paths();
	bool is_updated = true;
	while (is_updated) {
		is_updated = false;
		auto new_best_path_for_start = best_path_for_start;
		for (int floyd_start = 0; floyd_start < graph_data.vertex_count; ++floyd_start) {
			for (const auto &[start_index, path]: best_path_for_start) {
				const auto &[vertexes, length] = floyd_ways[floyd_start][start_index];
				if (!std::isfinite(length)) continue;
				if (const auto it = new_best_path_for_start.find(floyd_start);
					it == new_best_path_for_start.end() || length + path.length < it->second.length) {
					is_updated = true;
					auto new_best_path = Path({}, 0);
					new_best_path += floyd_ways[floyd_start][start_index];
					new_best_path += path;
					new_best_path_for_start[floyd_start] = new_best_path;
				}
			}
		}
		best_path_for_start = std::move(new_best_path_for_start);
	}
}

void QuestOptimizer::optimize() {
	for (int i = 0; i < graph_data.vertex_count; ++i) {
		queue.insert(PathState(i, Path{{}, 0.0}, graph_data.quest_lines));
	}
	auto threads = std::vector<std::thread>{};
	threads.reserve(num_threads);
	for (unsigned i = 0; i < num_threads; ++i) {
		threads.emplace_back(&QuestOptimizer::optimize_cycle, this);
	}
	for (unsigned i = 0; i < num_threads; ++i) {
		threads[i].join();
	}
	optimize_floyd();
	if (graph_data.start_index == -1) {
		best_path = std::ranges::min_element(best_path_for_start
											,
											[](const auto &a, const auto &b) {
												return a.second.vertexes.size() < b.second.vertexes.size();
											})->second;
	} else {
		best_path = best_path_for_start[graph_data.start_index];
	}
}

Path QuestOptimizer::get_best_path() const {
	return best_path;
}

void QuestOptimizer::print_quests_on_path() {
	std::cout << best_path.length << ENDL;
	std::vector<std::pair<int, QuestLine> > quest_lines_dict{};
	for (size_t i = 0; i < graph_data.quest_lines.size(); ++i) {
		quest_lines_dict.emplace_back(i, graph_data.quest_lines[i]);
	}
	for (const auto vertex_index: best_path.vertexes) {
		std::cout << vertex_index << ':';
		for (auto it = quest_lines_dict.begin(); it != quest_lines_dict.end();) {
			auto &[i, quest_line] = *it;
			while (!quest_line.vertexes.empty() && quest_line.vertexes.front() == vertex_index) {
				std::cout << i << ' ';
				quest_line.vertexes.pop_front();
			}
			if (quest_line.vertexes.empty()) {
				it = quest_lines_dict.erase(it);
			} else {
				++it;
			}
		}
		std::cout << ENDL;
	}
}
