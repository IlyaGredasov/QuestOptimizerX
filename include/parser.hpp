#pragma once

#include <list>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

class InvalidFormat final : public std::runtime_error {
public:
	explicit InvalidFormat(const std::string &msg) : std::runtime_error(msg) {}
};

struct QuestLine {
	int id{};
	std::string name;
	std::list<int> vertexes;
};

struct GraphData {
	std::vector<std::vector<double>> adj_list;
	bool fast_travel;
	bool weighted;
	bool bidirectional;
	int vertex_count;
	int start_index = -1;
	std::vector<std::string> vertex_names;
	std::vector<QuestLine> quest_lines;
};

using LineIter = std::vector<std::string>::iterator;

class Parser {
public:
	static GraphData parse_file(const std::string &file_path);

private:
	static std::vector<std::string> lines;
	static LineIter current_line;
	static GraphData graph_data;

	static void read_file(const std::string &file_path);

	static const std::unordered_map<std::string, void(*)()> string_to_parse_func;

	static void parse_fast_travel();

	static void parse_bidirectional();

	static void parse_weighted();

	static void parse_vertex_count();

	static void parse_vertexes();

	static void parse_edges();

	static void parse_quest_lines();

	static void parse_start();
};
