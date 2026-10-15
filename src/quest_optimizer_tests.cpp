#include <cmath>
#include <filesystem>
#include <vector>

#include <gtest/gtest.h>
#include <yaml-cpp/yaml.h>

#include "qo/parser.hpp"
#include "qo/quest_optimizer_x.hpp"

namespace qo {
namespace {
    struct TestContext {
        std::filesystem::path yaml_path;

        GraphData graph_data;

        explicit TestContext(const std::string& file_name)
            : yaml_path(std::filesystem::current_path() / "tests" / file_name),

              graph_data(Parser::parse_file(yaml_path.string())) {}

        bool check_quest_completion(const Path& path) const {
            std::vector<size_t> quest_positions(graph_data.quest_lines.size(), 0);

            for (int vertex : path.vertexes) {
                for (size_t q_id = 0; q_id < graph_data.quest_lines.size(); ++q_id) {
                    const auto& ql = graph_data.quest_lines[q_id];
                    if (quest_positions[q_id] >= ql.vertexes.size())
                        continue;

                    while (quest_positions[q_id] < ql.vertexes.size() && ql.vertexes[quest_positions[q_id]] == vertex) {
                        ++quest_positions[q_id];
                    }
                }
            }

            for (size_t i = 0; i < graph_data.quest_lines.size(); ++i)
                if (quest_positions[i] != graph_data.quest_lines[i].vertexes.size())
                    return false;

            return true;
        }

        static bool check_length(const Path& path, double expected_len, double eps = 0.05) {
            if (std::isnan(expected_len))
                return false;
            return std::abs(path.length - expected_len) <= eps * expected_len;
        }
    };

    TEST(QuestOptimizerTest, LinearGraphSingleQuest) {
        TestContext ctx("test1.yaml");

        QuestOptimizer optimizer(
            ctx.graph_data,
            /*threads*/ 4,
            /*max_queue_size*/ 100000,
            /*error_afford*/ 1.05,
            /*depth_of_search*/ 1,
            /*log_interval_seconds*/ 0.1f
        );

        optimizer.optimize();
        const auto& best_path = optimizer.get_best_path();

        ASSERT_FALSE(best_path.vertexes.empty());
        EXPECT_TRUE(ctx.check_quest_completion(best_path));
        EXPECT_DOUBLE_EQ(best_path.length, 9.0);
    }

    TEST(QuestOptimizerTest, ShortestPathSelection) {
        TestContext ctx("test2.yaml");

        QuestOptimizer optimizer(ctx.graph_data, 4, 100000, 1.05, 1, 0.1f);
        optimizer.optimize();
        const auto& best_path = optimizer.get_best_path();

        ASSERT_FALSE(best_path.vertexes.empty());
        EXPECT_TRUE(ctx.check_quest_completion(best_path));
        EXPECT_FLOAT_EQ(best_path.length, 3.0);
    }

    TEST(QuestOptimizerTest, UnweightedMultiQuest) {
        TestContext ctx("test3.yaml");

        QuestOptimizer optimizer(ctx.graph_data, 4, 100000, 1.05, 1, 0.1f);
        optimizer.optimize();
        const auto& best_path = optimizer.get_best_path();

        ASSERT_FALSE(best_path.vertexes.empty());
        EXPECT_TRUE(ctx.check_quest_completion(best_path));
        EXPECT_GT(best_path.length, 0.0);
    }

    TEST(QuestOptimizerTest, BidirectionalGraph) {
        TestContext ctx("test4.yaml");

        QuestOptimizer optimizer(ctx.graph_data, 4, 100000, 1.05, 1, 0.1f);
        optimizer.optimize();
        const auto& best_path = optimizer.get_best_path();

        ASSERT_FALSE(best_path.vertexes.empty());
        EXPECT_TRUE(ctx.check_quest_completion(best_path));
    }

    TEST(QuestOptimizerTest, FastTravel) {
        TestContext ctx("test5.yaml");

        QuestOptimizer optimizer(ctx.graph_data, 4, 100000, 1.05, 1, 0.1f);
        optimizer.optimize();
        const auto& best_path = optimizer.get_best_path();

        ASSERT_FALSE(best_path.vertexes.empty());
        EXPECT_TRUE(ctx.check_quest_completion(best_path));
        EXPECT_GT(best_path.length, 0.0); // Внутри квеста «прыжок» ~1
    }

    TEST(QuestOptimizerTest, ComplexMultiPath) {
        TestContext ctx("test6.yaml");

        QuestOptimizer optimizer(ctx.graph_data, 4, 100000, 1.05, 1, 0.1f);
        optimizer.optimize();
        const auto& best_path = optimizer.get_best_path();

        ASSERT_FALSE(best_path.vertexes.empty());
        EXPECT_TRUE(ctx.check_quest_completion(best_path));
        EXPECT_FLOAT_EQ(best_path.length, 10.0);
    }

} // namespace
} // namespace qo
