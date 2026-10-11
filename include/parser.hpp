#pragma once

#include <stdexcept>
#include <string>
#include <vector>

class InvalidFormat final : public std::runtime_error {
public:
    explicit InvalidFormat(const std::string& msg) : std::runtime_error(msg) {}
};

struct QuestLine {
    int id{};
    std::string name;
    std::vector<int> vertexes;

    QuestLine() = default;
    QuestLine(const int id_value, const std::string& name_value) : id(id_value), name(name_value) {}
};

struct Edge {
    int to;
    double weight;
};

struct GraphData {
    std::vector<std::vector<Edge>> adj_list;
    bool fast_travel = false;
    bool weighted = false;
    bool bidirectional = false;
    int vertex_count = 0;
    int start_index = -1;
    std::vector<std::string> vertex_names;
    std::vector<QuestLine> quest_lines;
};

class Parser final {
public:
    static GraphData parse_file(const std::string& file_path);
};
