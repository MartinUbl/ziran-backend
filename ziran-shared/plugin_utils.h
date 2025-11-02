#pragma once

#include <string>
#include <map>
#include <vector>

namespace ziran {
	using ParamMap = std::map<std::string, std::string>;

	// converts two arrays of key-value components to a single map
	ParamMap Map_From_Param_Arrays(const char** keys, const char** values, const size_t sizes);

	bool Param_Get(const ParamMap& map, const std::string& key, std::string& target);

	namespace string {
		// trims leading and trailing spaces from a string
		std::string trim(const std::string& s);

		// trims leading and trailing newlines from a string
		std::string trim_newlines(const std::string& s);

		// trims all leading whitespaces (spaces, tabs and newlines)
		std::string trim_whitespaces(const std::string& s);

		// implodes a vector of strings to a string
		std::string implode(const std::vector<std::string>& vec, const std::string& delimiter = ", ");

		// replaces all occurrences of a substring in a string with another substring
		std::string replace_all(const std::string& str, const std::string& from, const std::string& to);

		// finds a difference between two strings and returns a string with the differences highlighted in HTML format
		std::string stringDiffHTML(const std::string& a, const std::string& b);
		std::string stringWordDiffHTML(const std::string& a, const std::string& b);
	}

	namespace proc {
		struct CommandResult {
			std::string output;
			int exitstatus;
		};

		constexpr int InvalidExitStatus = 255;

		CommandResult execute(const std::string& command, bool redirectStderrToStdout = true, const std::string& inputStreamContents = "");
	}
}
