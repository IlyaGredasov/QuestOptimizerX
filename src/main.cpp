#include <algorithm>
#include <iostream>
#include <string>
#include <thread>

#include <CLI/CLI.hpp>

#include "quest_optimizer_x.hpp"

int main(const int argc, char** argv) {
    try {
        CLI::App app{"Find an efficient route through ordered quest objectives"};

        std::string file;
        unsigned num_threads = std::max(1U, std::thread::hardware_concurrency());
        unsigned max_queue_size = 100000;
        double error_afford = 1.05;
        unsigned depth_of_search = 1;
        float log_interval_seconds = 1.0F;
        bool enable_vertex_names = false;
        bool enable_quest_line_names = false;

        app.add_option("-f,--file", file, "Input graph in YAML format")->required()->check(CLI::ExistingFile);
        app.add_option("--num_threads,--num-threads", num_threads, "Number of optimizer threads")
            ->check(CLI::PositiveNumber)
            ->capture_default_str();
        app.add_option("--max_queue_size,--max-queue-size", max_queue_size, "Maximum number of queued states")
            ->check(CLI::PositiveNumber)
            ->capture_default_str();
        app.add_option("--error_afford,--error-afford", error_afford, "Search pruning coefficient")
            ->check(CLI::PositiveNumber)
            ->capture_default_str();
        app.add_option("--depth_of_search,--depth-of-search", depth_of_search, "Number of complete paths to find")
            ->check(CLI::PositiveNumber)
            ->capture_default_str();
        app.add_option(
               "--log_interval_seconds,--log-interval-seconds",
               log_interval_seconds,
               "Logging interval in seconds; zero disables periodic logging"
        )
            ->check(CLI::NonNegativeNumber)
            ->capture_default_str();
        app.add_flag("--enable_vertex_names,--enable-vertex-names", enable_vertex_names, "Print vertex names");
        app.add_flag(
            "--enable_quest_line_names,--enable-quest-line-names",
            enable_quest_line_names,
            "Print quest line names"
        );

        try {
            app.parse(argc, argv);
        } catch (const CLI::ParseError& error) { return app.exit(error); }

        const auto graph_data = Parser::parse_file(file);
        QuestOptimizer optimizer(
            graph_data,
            num_threads,
            max_queue_size,
            error_afford,
            depth_of_search,
            log_interval_seconds
        );
        optimizer.optimize();
        print_quests_on_path(
            optimizer.get_best_path(),
            graph_data.quest_lines,
            graph_data.vertex_names,
            enable_vertex_names,
            enable_quest_line_names
        );
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] " << e.what() << '\n';
        return 1;
    }
    return 0;
}
