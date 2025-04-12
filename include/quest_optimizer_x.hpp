#ifndef QUEST_OPTIMIZER_X_HPP
#define QUEST_OPTIMIZER_X_HPP

#include <parser.hpp>

#include <vector>
#include <mutex>
#include <atomic>
#include <numeric>
#include <unordered_map>
#include <thread>

#include <ext/pb_ds/assoc_container.hpp>
#include <ext/pb_ds/tree_policy.hpp>

template<typename T>
using OrderedSet = __gnu_pbds::tree<
	T,
	__gnu_pbds::null_type,
	std::less<T>,
	__gnu_pbds::rb_tree_tag,
	__gnu_pbds::tree_order_statistics_node_update
>;

struct Path {
	std::vector<int> vertexes{};
	double length{};

	Path &operator+=(const Path &other) {
		vertexes.insert(vertexes.end(), other.vertexes.begin(), other.vertexes.end());
		length += other.length;
		return *this;
	}
};

int remain_quests(const std::vector<QuestLine> &quest_lines);

class QuestOptimizer {
public:
	explicit QuestOptimizer(const GraphData &graph_data,
							const bool fast_travel = false,
							const unsigned num_threads = std::thread::hardware_concurrency(),
							const int max_queue_size = 100000,
							const double error_afford = 1.05) : graph_data(graph_data),
																fast_travel(fast_travel),
																num_threads(num_threads),
																max_queue_size(max_queue_size),
																error_afford(error_afford) {
		minimum_quest_count = remain_quests(graph_data.quest_lines);
		for (int i = 0; i < graph_data.vertex_count; ++i) {
			best_path_for_start[i] = Path({},std::numeric_limits<double>::infinity());
		}
	};

	void optimize();

	Path get_best_path() const;

	void print_quests_on_path() const;

private:
	const GraphData &graph_data;
	bool fast_travel;
	unsigned num_threads;
	int max_queue_size;
	double error_afford;
	int minimum_quest_count;

	struct PathState {
		int current_index;
		Path path;
		std::vector<QuestLine> quests;

		bool operator<(const PathState &other) const {
			const auto first = remain_quests(quests);
			const auto second = remain_quests(other.quests);
			if (first != second) {
				return first < second;
			}
			if (path.length != other.path.length)
				return path.length < other.path.length;

			return current_index < other.current_index;
		}
	};

	OrderedSet<PathState> queue{};
	std::unordered_map<int, Path> best_path_for_start{};
	Path best_path = {{},std::numeric_limits<double>::infinity()};

	std::mutex queue_mutex{};
	std::atomic<bool> stop_event{false};

	std::vector<std::vector<Path> > get_floyd_paths() const;

	void optimize_cycle();

	void optimize_step();

	void optimize_floyd();
};

#endif //QUEST_OPTIMIZER_X_HPP
