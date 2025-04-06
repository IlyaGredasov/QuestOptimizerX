#include <fstream>
#include <parser.hpp>

GraphData Parser::parse_file(const std::string& file_path) {
	if (const std::ifstream infile(file_path); !infile) {
		throw InvalidFormat("Unable to open file: " + file_path);
	}

	GraphData data{};
	auto parser_state = ParserState::None;

	return {};
}
