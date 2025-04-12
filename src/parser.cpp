#include <parser.hpp>
#include <filesystem>
#include <algorithm>
#include <sstream>
#include <fstream>
#include <iostream>

namespace fs = std::filesystem;

std::vector<std::string> Parser::lines{};
LineIter Parser::current_line{};
GraphData Parser::graph_data{};

void Parser::read_file(const std::string &file_path) {
	std::cout << "Reading file " << file_path << std::endl;
	const fs::path file_path_(file_path);
	std::ifstream infile(file_path_);
	if (!infile) {
		throw InvalidFormat("Unable to open file: " + file_path);
	}
	std::string raw;
	while (std::getline(infile, raw)) {
		if (!raw.empty() && raw.back() == '\r') raw.pop_back();
		lines.push_back(raw);
	}
}

std::vector<std::string> extract_words(const std::string &input) {
	std::vector<std::string> words;
	std::istringstream stream(input);
	std::string word;
	while (stream >> word) {
		words.push_back(word);
	}
	return words;
}

void Parser::parse_bidirectional() {
	std::cout << "Parsing bidirectional" << std::endl;
	if (*current_line == "\tTrue") {
		graph_data.bidirectional = true;
	} else if (*current_line == "\tFalse") {
		graph_data.bidirectional = false;
	} else {
		throw InvalidFormat("Invalid Format Of <Bidirectional> statement");
	}
}

void Parser::parse_weighted() {
	std::cout << "Parsing weighted" << std::endl;
	if (*current_line == "\tTrue") {
		graph_data.weighted = true;
	} else if (*current_line == "\tFalse") {
		graph_data.weighted = false;
	} else {
		throw InvalidFormat("Invalid Format Of <Weighted> statement");
	}
}

void Parser::parse_vertex_count() {
	std::cout << "Parsing vertex count" << std::endl;
	if (const auto value = current_line->substr(1); !value.empty() && std::ranges::all_of(value, isdigit)) {
		const auto vertex_count = std::stoi(value);
		if (vertex_count < 0) {
			throw InvalidFormat("<VertexCount> is less then 0");
		}
		graph_data.vertex_count = vertex_count;
	} else {
		throw InvalidFormat("Invalid Format Of <Start> statement");
	}
	graph_data.vertex_names.resize(graph_data.vertex_count);
	graph_data.adj_list = std::vector(graph_data.vertex_count,
									std::vector(graph_data.vertex_count, std::numeric_limits<double>::infinity()));
}

void Parser::parse_vertexes() {
	std::cout << "Parsing vertexes" << std::endl;
	if (!graph_data.vertex_count) {
		throw InvalidFormat("<VertexCount> hasn't been defined yet");
	}
	for (int i = 0; current_line != lines.end() && !string_to_parse_func.contains(*current_line); ++i, ++current_line) {
		std::cout << *current_line << std::endl;
		const auto words = extract_words(*current_line);
		if (const int index = std::stoi(words[0]); index < 0) {
			throw InvalidFormat("Vertex index is less then 0");
		}
		const std::string &vertex_name = words[1];
		graph_data.vertex_names[i] = vertex_name;
		graph_data.vertex_name_to_index[vertex_name] = i;
	}
	--current_line;
}

void Parser::parse_edges() {
	std::cout << "Parsing edges" << std::endl;
	for (int i = 0; current_line != lines.end() && !string_to_parse_func.contains(*current_line); ++i, ++current_line) {
		const auto words = std::move(extract_words(*current_line));
		if (words.size() != 2 && words.size() != 3) {
			throw InvalidFormat("Invalid Format in <Edges> statement");
		}
		if (!graph_data.vertex_count) {
			throw InvalidFormat("<VertexCount> hasn't been defined yet");
		}
		if (words.size() == 3 && !graph_data.weighted) {
			throw InvalidFormat("<Weighted> is False, but <Edges> statement contains 3 tokens");
		}
		if (words.size() == 2 && graph_data.weighted) {
			throw InvalidFormat("<Weighted> is True, but <Edges> statement contains 2 tokens");
		}
		const auto first = std::stoi(words[0]);
		const auto second = std::stoi(words[1]);
		const auto edge = graph_data.weighted ? std::stod(words[2]) : 1.0;
		if (first < 0 || first > graph_data.vertex_count - 1 || second < 0 || second > graph_data.vertex_count - 1) {
			throw InvalidFormat("Vertex index is out of range");
		}
		if (edge < 0) {
			throw InvalidFormat("Edge length is less then 0");
		}
		graph_data.adj_list[first][second] = edge;
		if (graph_data.bidirectional) {
			graph_data.adj_list[second][first] = edge;
		}
	}
	--current_line;
}

void Parser::parse_quest_lines() {
	std::cout << "Parsing quest lines" << std::endl;
	for (int i = 0; current_line != lines.end() && !string_to_parse_func.contains(*current_line); ++i, ++current_line) {
		std::vector<std::string> words = std::move(extract_words(*current_line));
		auto vertexes_end_it = words.end();
		if (!std::ranges::all_of(words[words.size() - 1], isdigit)) {
			--vertexes_end_it;
		}
		QuestLine ql;
		for (auto it = words.begin(); it != vertexes_end_it; ++it) {
			if (!std::ranges::all_of(*it, isdigit)) {
				throw InvalidFormat("Invalid Format Of <QuestLine> statement");
			}
			ql.vertexes.emplace_back(std::stoi(*it));
		}
		if (vertexes_end_it != words.end()) {
			graph_data.quest_line_name_to_index[words[words.size() - 1]] = i;
		}
		graph_data.quest_lines.push_back(std::move(ql));
	}
	--current_line;
}

void Parser::parse_start() {
	std::cout << "Parsing start" << std::endl;
	++current_line;
	std::cout << *current_line << std::endl;
	if (const auto value = current_line->substr(1); !value.empty() && std::ranges::all_of(value, isdigit)) {
		const auto start_index = std::stoi(value.substr(1));
		if (!graph_data.vertex_count) {
			throw InvalidFormat("<VertexCount> hasn't been defined yet");
		}
		if (start_index < 0 || start_index > graph_data.vertex_count) {
			throw InvalidFormat("<StartIndex> is out of range");
		}
		graph_data.start_index = start_index;
	} else {
		throw InvalidFormat("Invalid Format Of <Start> statement");
	}
}

const std::unordered_map<std::string, void(*)()> Parser::string_to_parse_func = {
	{"Bidirectional:", parse_bidirectional},
	{"Weighted:", parse_weighted},
	{"VertexCount:", parse_vertex_count},
	{"Vertexes:", parse_vertexes},
	{"Edges:", parse_edges},
	{"QuestLines:", parse_quest_lines},
	{"Start:", parse_start}
};

GraphData Parser::parse_file(const std::string &file_path) {
	read_file(file_path);
	for (current_line = lines.begin(); current_line != lines.end(); ++current_line) {
		if (string_to_parse_func.contains(*current_line)) {
			const auto parse_func = *string_to_parse_func.at(*current_line);
			++current_line;
			parse_func();
		}
	}
	return graph_data;
}
