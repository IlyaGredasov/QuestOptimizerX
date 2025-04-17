#include <iostream>
#include <quest_optimizer_x.hpp>

std::unordered_map<std::string, std::string> parse_args(const int argc, char **argv) {
	std::unordered_map<std::string, std::string> args;
	for (int i = 1; i < argc; ++i) {
		if (std::string key = argv[i];
			key.starts_with("--")) {
			if (i + 1 < argc && !std::string(argv[i + 1]).starts_with("--")) {
				args[key] = argv[i + 1];
				++i;
			} else {
				args[key] = "";
			}
		}
	}
	return args;
}


int main(const int argc, char **argv) {
	try {
		auto args = parse_args(argc, argv);
		const std::string file = args["--file"];
		const auto graph_data = Parser::parse_file(file);
		QuestOptimizer optimizer(
			graph_data,
			std::stoi(args["--num_threads"]),
			std::stoi(args["--max_queue_size"]),
			std::stod(args["--error_afford"]),
			std::stoi(args["--deep_of_search"]),
			std::stod(args["--queue_narrowness"]),
			std::stof(args["--log_interval_seconds"])
		);
		optimizer.optimize();
		print_quests_on_path(optimizer.get_best_path(), graph_data.quest_lines, graph_data.vertex_names,
							!args.contains("--disable_vertex_names"),
							!args.contains("--disable_quest_line_names"));
	} catch (const std::exception &e) {
		std::cerr << "[ERROR] " << e.what() << std::endl;
		return 1;
	}
	return 0;
}
