#include <iostream>
#include <parser.hpp>
#include <quest_optimizer_x.hpp>

int main() {
	try {
		const auto graph_data = Parser::parse_file("../example.txt");
		QuestOptimizer optimizer(graph_data, false, std::thread::hardware_concurrency(), 100000, 1.00);
		optimizer.optimize();
		for (const auto [vertexes, length] = optimizer.get_best_path(); auto &vertex: vertexes) {
			std::cout << vertex << ' ';
		}
	} catch (const std::exception &e) {
		std::cout << e.what() << std::endl;
	}
	return 0;
}
