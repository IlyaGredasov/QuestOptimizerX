#include <algorithm>
#include <cmath>
#include <limits>
#include <string>
#include <utility>

#include <yaml-cpp/yaml.h>

#include "parser.hpp"

namespace {
    template <typename Value>
    Value required_value(const YAML::Node& node, const char* key) {
        const auto value = node[key];
        if (!value) {
            throw InvalidFormat("Missing required YAML field: " + std::string(key));
        }
        return value.as<Value>();
    }

    void require_map(const YAML::Node& node, const std::string& context) {
        if (!node.IsMap()) {
            throw InvalidFormat(context + " must be a YAML mapping");
        }
    }

    void require_sequence(const YAML::Node& node, const std::string& context) {
        if (!node || !node.IsSequence()) {
            throw InvalidFormat(context + " must be a YAML sequence");
        }
    }

    void validate_vertex(const int vertex, const int vertex_count, const std::string& context) {
        if (vertex < 0 || vertex >= vertex_count) {
            throw InvalidFormat(context + " vertex index is out of range: " + std::to_string(vertex));
        }
    }

    void sort_and_merge_edges(std::vector<Edge>& edges) {
        std::ranges::stable_sort(edges, {}, &Edge::to);
        size_t write_index = 0;
        for (const auto& edge : edges) {
            if (write_index > 0 && edges[write_index - 1].to == edge.to) {
                edges[write_index - 1].weight = edge.weight;
            } else {
                edges[write_index++] = edge;
            }
        }
        edges.resize(write_index);
    }
} // namespace

GraphData Parser::parse_file(const std::string& file_path) {
    try {
        const auto root = YAML::LoadFile(file_path);
        require_map(root, "YAML document root");

        GraphData graph_data;
        graph_data.fast_travel = required_value<bool>(root, "fast_travel");
        graph_data.bidirectional = required_value<bool>(root, "bidirectional");
        graph_data.weighted = required_value<bool>(root, "weighted");
        graph_data.vertex_count = required_value<int>(root, "vertex_count");
        if (graph_data.vertex_count <= 0) {
            throw InvalidFormat("vertex_count must be greater than zero");
        }

        graph_data.adj_list.resize(static_cast<size_t>(graph_data.vertex_count));
        graph_data.vertex_names.resize(static_cast<size_t>(graph_data.vertex_count));
        for (int vertex = 0; vertex < graph_data.vertex_count; ++vertex) {
            graph_data.vertex_names[static_cast<size_t>(vertex)] = "vertex_" + std::to_string(vertex);
        }

        if (const auto start = root["start"]) {
            graph_data.start_index = start.as<int>();
            validate_vertex(graph_data.start_index, graph_data.vertex_count, "start");
        }

        if (const auto vertices = root["vertices"]) {
            require_sequence(vertices, "vertices");
            for (const auto& vertex_node : vertices) {
                require_map(vertex_node, "vertices item");
                const auto id = required_value<int>(vertex_node, "id");
                validate_vertex(id, graph_data.vertex_count, "vertices item");
                graph_data.vertex_names[static_cast<size_t>(id)] = required_value<std::string>(vertex_node, "name");
            }
        }

        const auto edges = root["edges"];
        require_sequence(edges, "edges");
        for (const auto& edge_node : edges) {
            require_map(edge_node, "edges item");
            const auto from = required_value<int>(edge_node, "from");
            const auto to = required_value<int>(edge_node, "to");
            validate_vertex(from, graph_data.vertex_count, "edge source");
            validate_vertex(to, graph_data.vertex_count, "edge destination");

            const auto weight_node = edge_node["weight"];
            if (graph_data.weighted && !weight_node) {
                throw InvalidFormat("Weighted edge is missing the weight field");
            }
            if (!graph_data.weighted && weight_node) {
                throw InvalidFormat("Unweighted edge must not contain the weight field");
            }
            const auto weight = graph_data.weighted ? weight_node.as<double>() : 1.0;
            if (!std::isfinite(weight) || weight < 0.0) {
                throw InvalidFormat("Edge weight must be finite and non-negative");
            }

            graph_data.adj_list[static_cast<size_t>(from)].push_back({to, weight});
            if (graph_data.bidirectional) {
                graph_data.adj_list[static_cast<size_t>(to)].push_back({from, weight});
            }
        }
        for (auto& vertex_edges : graph_data.adj_list) {
            sort_and_merge_edges(vertex_edges);
        }

        const auto quest_lines = root["quest_lines"];
        require_sequence(quest_lines, "quest_lines");
        if (quest_lines.size() > static_cast<size_t>(std::numeric_limits<int>::max())) {
            throw InvalidFormat("Too many quest lines");
        }
        graph_data.quest_lines.reserve(quest_lines.size());
        for (size_t index = 0; index < quest_lines.size(); ++index) {
            const auto quest_node = quest_lines[index];
            require_map(quest_node, "quest_lines item");

            const auto id = quest_node["id"] ? quest_node["id"].as<int>() : static_cast<int>(index);
            const auto default_name = "quest_" + std::to_string(id);
            const auto name = quest_node["name"] ? quest_node["name"].as<std::string>() : default_name;
            const auto quest_vertices = quest_node["vertices"];
            require_sequence(quest_vertices, "quest line vertices");

            QuestLine quest_line(id, name);
            quest_line.vertexes.reserve(quest_vertices.size());
            for (const auto& vertex_node : quest_vertices) {
                const auto vertex = vertex_node.as<int>();
                validate_vertex(vertex, graph_data.vertex_count, "quest line");
                quest_line.vertexes.push_back(vertex);
            }
            graph_data.quest_lines.push_back(std::move(quest_line));
        }

        return graph_data;
    } catch (const InvalidFormat&) { throw; } catch (const YAML::Exception& error) {
        throw InvalidFormat("Invalid YAML in " + file_path + ": " + error.what());
    }
}

