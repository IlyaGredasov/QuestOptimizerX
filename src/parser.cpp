#include <parser.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

namespace fs = std::filesystem;

std::vector<std::string> Parser::lines{};
LineIter Parser::current_line{};
GraphData Parser::graph_data{};

void Parser::read_file(const std::string &file_path) {
	std::cout << "Reading file " << file_path << "\n";
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

void Parser::parse_fast_travel() {
	std::cout << "Parsing fast_travel\n";
	if (*current_line == "\tTrue") {
		graph_data.fast_travel = true;
	} else if (*current_line == "\tFalse") {
		graph_data.fast_travel = false;
	} else {
		throw InvalidFormat("Invalid Format Of <FastTravel> statement");
	}
}

void Parser::parse_bidirectional() {
	std::cout << "Parsing bidirectional\n";
	if (*current_line == "\tTrue") {
		graph_data.bidirectional = true;
	} else if (*current_line == "\tFalse") {
		graph_data.bidirectional = false;
	} else {
		throw InvalidFormat("Invalid Format Of <Bidirectional> statement");
	}
}

void Parser::parse_weighted() {
	std::cout << "Parsing weighted\n";
	if (*current_line == "\tTrue") {
		graph_data.weighted = true;
	} else if (*current_line == "\tFalse") {
		graph_data.weighted = false;
	} else {
		throw InvalidFormat("Invalid Format Of <Weighted> statement");
	}
}

void Parser::parse_vertex_count() {
	std::cout << "Parsing vertex count\n";
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
	std::cout << "Parsing vertexes\n";
	if (!graph_data.vertex_count)
		throw InvalidFormat("<VertexCount> hasn't been defined yet");

	graph_data.vertex_names.resize(graph_data.vertex_count);
	for (int i = 0; i < graph_data.vertex_count; ++i) {
		graph_data.vertex_names[i] = "vertex_" + std::to_string(i);
	}

	for (int i = 0; current_line != lines.end() && !string_to_parse_func.contains(*current_line); ++i, ++current_line) {
		const auto words = extract_words(*current_line);
		const int index = std::stoi(words[0]);

		if (index < 0 || index >= graph_data.vertex_count)
			throw InvalidFormat("Vertex index is out of bounds");

		if (words.size() > 1) {
			graph_data.vertex_names[index] = words[1];
		}
	}
	--current_line;
}

void Parser::parse_edges() {
	std::cout << "Parsing edges\n";
	for (int i = 0; current_line != lines.end() && !string_to_parse_func.contains(*current_line); ++i, ++current_line) {
		const auto words = extract_words(*current_line);
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
	std::cout << "Parsing quest lines\n";
	for (int i = 0; current_line != lines.end() && !string_to_parse_func.contains(*current_line); ++i, ++current_line) {
		std::vector<std::string> words = extract_words(*current_line);
		auto vertexes_end_it = words.end();
		std::string name = "quest_" + std::to_string(i);
		if (!std::ranges::all_of(words.back(), isdigit)) {
			name = words.back();
			--vertexes_end_it;
		}
		QuestLine ql{.id = i, .name = std::move(name)};
		for (auto it = words.begin(); it != vertexes_end_it; ++it) {
			if (!std::ranges::all_of(*it, isdigit))
				throw InvalidFormat("Invalid Format Of <QuestLine> statement");
			ql.vertexes.push_back(std::stoi(*it));
		}
		graph_data.quest_lines.push_back(std::move(ql));
	}
	--current_line;
}

void Parser::parse_start() {
	std::cout << "Parsing start\n";
	if (const auto value = current_line->substr(1); !value.empty() && std::ranges::all_of(value, isdigit)) {
		const auto start_index = std::stoi(value);
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
	{"FastTravel:", parse_fast_travel},
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
