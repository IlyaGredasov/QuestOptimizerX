#ifndef QUEST_OPTIMIZER_X_HPP
#define QUEST_OPTIMIZER_X_HPP

#include <parser.hpp>

#include <atomic>
#include <condition_variable>
#include <limits>
#include <mutex>
#include <numeric>
#include <ranges>
#include <thread>
#include <unordered_map>
#include <vector>

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
	double length = 0.0;

	Path &operator+=(const Path &other) {
		vertexes.insert(vertexes.end(), other.vertexes.begin(), other.vertexes.end());
		length += other.length;
		return *this;
	}
};

int remain_quests(const std::vector<QuestLine> &quest_lines);

bool print_quests_on_path(
	const Path &path,
	const std::vector<QuestLine> &quest_lines,
	const std::vector<std::string> &vertex_names,
	bool use_vertex_names,
	bool use_quest_names
);

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

class QuestOptimizer final {
public:
	explicit QuestOptimizer(const GraphData &graph_data,
							const unsigned num_threads = std::thread::hardware_concurrency(),
							const unsigned max_queue_size = 100000,
							const double error_afford = 1.05,
							const unsigned deep_of_search = 1,
							const double queue_narrowness = 0.5,
							const float log_interval_seconds = 1.0) : graph_data(graph_data),
																	num_threads(num_threads),
																	max_queue_size(max_queue_size),
																	error_afford(error_afford),
																	deep_of_search(deep_of_search),
																	queue_narrowness(queue_narrowness),
																	log_interval_seconds(log_interval_seconds) {
		minimum_quest_count = remain_quests(graph_data.quest_lines);
		std::ranges::for_each(
			std::views::iota(0, graph_data.vertex_count),
			[&](const int i) {
				best_path_for_start[i] = Path({}, std::numeric_limits<double>::infinity());
			}
		);
	};

	void optimize();

	Path get_best_path() const;

private:
	const GraphData &graph_data;
	const unsigned num_threads;
	const unsigned max_queue_size;
	const double error_afford;
	const unsigned deep_of_search;
	const double queue_narrowness;
	const float log_interval_seconds;

	std::atomic<unsigned> found_best_paths = 0;
	std::atomic<unsigned> minimum_quest_count;
	std::unordered_map<int, Path> best_path_for_start{};
	Path best_path = {{}, std::numeric_limits<double>::infinity()};

	OrderedSet<PathState> queue{};
	std::condition_variable cv_queue;
	unsigned sleeping_threads = 0;
	std::mutex queue_mutex{};
	std::atomic<bool> stop_event{false};

	std::unordered_map<int, Path> dijkstra_from(int start) const;

	void optimize_cycle();
};

#endif //QUEST_OPTIMIZER_X_HPP
