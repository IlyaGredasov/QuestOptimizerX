#ifndef PARSER_HPP
#define PARSER_HPP
#include <stdexcept>
#include <string>
#include <vector>

struct QuestLine {
	int id;
	std::string name;
	std::vector<int> nodes;
};

enum class ParserState {
	None,
	Bidirectional,
	Weighted,
	VertexCount,
	Vertexes,
	Edges,
	Questlines,
	Start
};


class InvalidFormat final : public std::runtime_error {
public:
	explicit InvalidFormat(const std::string &msg) : std::runtime_error(msg) {
	}
};

class DisjointedGraph final : public std::runtime_error {
public:
	explicit DisjointedGraph(const std::string &msg) : std::runtime_error(msg) {
	}
};

struct GraphData {
	std::vector<std::vector<std::pair<int, double> > > adjList;
	bool weighted;
	bool bidirectional;
	int vertexCount;
	int startIndex;
	std::vector<std::string> vertexNames;
	std::vector<QuestLine> questlines;
};

class Parser {
public:
	static GraphData parse_file(const std::string &file_path);
};

#endif //PARSER_HPP
