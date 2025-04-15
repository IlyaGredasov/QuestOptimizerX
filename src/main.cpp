#include <quest_optimizer_x.hpp>
#include <future>
#include <limits>
#include <vector>
#include <iostream>

int main() {
	try {
		const auto graph_data = Parser::parse_file("../example.txt");

		// std::vector<std::future<Path>> futures;
		// for (int i = 0; i < 1; ++i) {
		// 	futures.emplace_back(std::async(std::launch::async, [&] {
		// 		QuestOptimizer optimizer(graph_data, false,
		// 			1, 100000, 1.00, 100);
		// 		optimizer.optimize();
		// 		return optimizer.get_best_path();
		// 	}));
		// }
		// std::pair<std::vector<int>, double> best_path_result;
		// double best_length = std::numeric_limits<double>::infinity();
		//
		// for (auto &future : futures) {
		// 	auto [vertexes, length] = future.get();
		// 	std::cout << length << ENDL;
		// 	if (length < best_length) {
		// 		best_length = length;
		// 		best_path_result = {vertexes, length};
		// 	}
		// }
		QuestOptimizer optimizer(graph_data, true, 1, 600000, 1.00, 1, 0.5);
		optimizer.optimize();
		optimizer.print_quests_on_path();


	} catch (const std::exception &e) {
		std::cerr << "[ERROR] " << e.what() << ENDL;
	}
}
