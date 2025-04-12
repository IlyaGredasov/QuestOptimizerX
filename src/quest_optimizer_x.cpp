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
		for (int i = 0; i < graph_data.adj_list.size(); ++i) {
			for (int j = 0; j < graph_data.adj_list.size(); ++j) {
				ways[i][j] = Path({i, j}, 1.0);
			}
		}
		return ways;
	}
	auto path_matrix = graph_data.adj_list;
	std::vector<std::vector<int> > next_node;
	for (int i2 = 0; i2 < path_matrix.size(); ++i2) {
		for (int i1 = 0; i1 < path_matrix.size(); ++i1) {
			for (int i3 = 0; i3 < path_matrix.size(); ++i3) {
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
	for (int i1 = 0; i1 < path_matrix.size(); ++i1) {
		for (int i2 = 0; i2 < path_matrix.size(); ++i2) {
			if (path_matrix[i1][i2] != std::numeric_limits<double>::infinity()) {
				auto way = Path({}, 0);
				int current = i1;
				while (current != i2) {
					way += Path({current}, path_matrix[current][i2]);
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
	while (!stop_event.load(std::memory_order_relaxed)) {
		optimize_step();
	}
}

void QuestOptimizer::optimize_step() {
	std::lock_guard lock{queue_mutex};
	if (queue.empty()) {
		return;
	}
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution dist(0, static_cast<int>(queue.size() - 1));
	auto [current_index, current_path, current_quests] = *queue.find_by_order(dist(gen));
	current_path += Path({current_index}, 0);
	if (remain_quests(current_quests) <= std::max(minimum_quest_count, 1) * error_afford) {
		std::cout << "minimum_quest_count: " << minimum_quest_count << " queue_size: " << queue.size() << std::endl;
		for (auto quest_line_it = current_quests.begin(); quest_line_it != current_quests.end();
			/* no increment here */) {
			if (!quest_line_it->vertexes.empty() && quest_line_it->vertexes.front() == current_index) {
				quest_line_it->vertexes.pop_front();
			}
			if (quest_line_it->vertexes.empty()) {
				quest_line_it = current_quests.erase(quest_line_it); // erase returns next valid iterator
			} else {
				++quest_line_it;
			}
		}
		if (current_quests.empty() && (!best_path_for_start.contains(current_path.vertexes[0]) || best_path_for_start[
											current_path.vertexes[0]].length > current_path.length)) {
			best_path_for_start[current_path.vertexes[0]] = current_path;
		} else if (fast_travel) {
			for (auto quest_line: current_quests) {
				auto new_state = PathState(current_index, current_path, current_quests);
				new_state.current_index = quest_line.vertexes.front();
				new_state.path.length += 1;
				queue.insert(new_state);
			}
		} else {
			for (int i = 0; i < graph_data.vertex_count; ++i) {
				if (graph_data.adj_list[current_index][i] != std::numeric_limits<double>::infinity()) {
					auto new_state = PathState(current_index, current_path, current_quests);
					new_state.current_index = i;
					new_state.path.length += graph_data.adj_list[current_index][i];
					if (queue.size() == max_queue_size) {
						queue.erase(queue.find_by_order(dist(gen)));
					}
					queue.insert(new_state);
				}
			}
		}
	}
	minimum_quest_count = std::min(minimum_quest_count, remain_quests(current_quests));
	if (!minimum_quest_count) {
		stop_event.store(true, std::memory_order_relaxed);
	}
}

void QuestOptimizer::optimize_floyd() {
	std::cout << "Floyd optimization" << std::endl;
	const auto floyd_ways = get_floyd_paths();
	bool is_updated = true;
	while (is_updated) {
		is_updated = false;
		auto new_best_path_for_start = best_path_for_start;
		for (int floyd_start = 0; floyd_start < graph_data.vertex_count; ++floyd_start) {
			for (const auto &[start_index, path]: best_path_for_start) {
				if (!best_path_for_start.contains(start_index) || floyd_ways[floyd_start][start_index].length + path.
					length < new_best_path_for_start[floyd_start].length) {
					is_updated = true;
					auto new_best_path = Path({}, 0);
					std::vector<int> reversed_floyd_way{};
					std::ranges::reverse_copy(floyd_ways[floyd_start][start_index].vertexes,
											std::back_inserter(reversed_floyd_way));
					new_best_path += Path(reversed_floyd_way, floyd_ways[floyd_start][start_index].length);
					new_best_path += path;
					new_best_path_for_start[floyd_start] = new_best_path;
				}
			}
		}
		best_path_for_start = std::move(new_best_path_for_start);
	}
}

void QuestOptimizer::optimize() {
	queue_mutex.lock();
	for (int i = 0; i < graph_data.vertex_count; ++i) {
		queue.insert(PathState(i, Path{}, graph_data.quest_lines));
	}
	std::cout << queue.size() << std::endl;
	queue_mutex.unlock();
	auto threads = std::vector<std::thread>{};
	threads.reserve(num_threads);
	for (int i = 0; i < num_threads; ++i) {
		std::cout << "Initializing thread " << i << std::endl;
		threads.emplace_back(&QuestOptimizer::optimize_cycle, this);
	}
	for (int i = 0; i < num_threads; ++i) {
		threads[i].join();
		std::cout << "Joining thread " << i << std::endl;
	}
	optimize_cycle();
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

void QuestOptimizer::print_quests_on_path() const {
	std::vector<std::pair<int, QuestLine> > quest_lines_dict{};
	for (int i = 0; i < graph_data.quest_lines.size(); ++i) {
		quest_lines_dict.emplace_back(i, graph_data.quest_lines[i]);
	}
	for (const auto &vertex_index: best_path.vertexes) {
	}
}
