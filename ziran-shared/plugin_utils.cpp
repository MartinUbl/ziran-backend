#include "plugin_utils.h"
#include "plugin.h"

#include <algorithm>
#include <array>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <unordered_map>

#undef max
#undef min

namespace ziran {

	bool operator&(NPlugin_Parameter_Flags a, NPlugin_Parameter_Flags b) {
		return (static_cast<uint8_t>(a) & static_cast<uint8_t>(b)) != 0;
	}

	NPlugin_Parameter_Flags operator|(NPlugin_Parameter_Flags a, NPlugin_Parameter_Flags b) {
		return static_cast<NPlugin_Parameter_Flags>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
	}

	ParamMap Map_From_Param_Arrays(const char** keys, const char** values, const size_t sizes) {
		if (!keys || !values || sizes == 0) {
			return {};
		}

		std::map<std::string, std::string> p;

		for (size_t i = 0; i < sizes; i++) {
			if (!keys[i]) {
				continue;
			}

			p[keys[i]] = values[i] ? values[i] : "";
		}

		return p;
	}

	bool Param_Get(const ParamMap& map, const std::string& key, std::string& target) {

		auto itr = map.find(key);
		if (itr == map.end()) {
			return false;
		}

		target = itr->second;
		return true;
	}

	namespace string {
		std::string trim(const std::string& s) {
			auto wsfront = std::find_if_not(s.begin(), s.end(), [](int c) {return std::isspace(c); });
			auto wsback = std::find_if_not(s.rbegin(), s.rend(), [](int c) {return std::isspace(c); }).base();
			return (wsback <= wsfront ? std::string() : std::string(wsfront, wsback));
		}

		std::string trim_newlines(const std::string& s) {
			auto wsfront = std::find_if_not(s.begin(), s.end(), [](int c) { return (c == '\r' || c == '\n'); });
			auto wsback = std::find_if_not(s.rbegin(), s.rend(), [](int c) { return (c == '\r' || c == '\n'); }).base();
			return (wsback <= wsfront ? std::string() : std::string(wsfront, wsback));
		}

		std::string trim_whitespaces(const std::string& s) {
			auto wsfront = std::find_if_not(s.begin(), s.end(), [](int c) { return (std::isspace(c) || c == '\t' || c == '\r' || c == '\n'); });
			auto wsback = std::find_if_not(s.rbegin(), s.rend(), [](int c) { return (std::isspace(c) || c == '\t' || c == '\r' || c == '\n'); }).base();
			return (wsback <= wsfront ? std::string() : std::string(wsfront, wsback));
		}

		std::string implode(const std::vector<std::string>& vec, const std::string& delimiter) {
			std::string out = "";

			for (size_t i = 0; i < vec.size(); i++) {
				if (i != 0) {
					out += delimiter;
				}
				out += vec[i];
			}

			return out;
		}

		std::string replace_all(const std::string& str, const std::string& from, const std::string& to) {
			std::string result = str;
			size_t start_pos = 0;
			while ((start_pos = result.find(from, start_pos)) != std::string::npos) {
				result.replace(start_pos, from.length(), to);
				start_pos += to.length();
			}
			return result;
		}

		// HTML escaping function
		static std::string escapeHTML(const std::string& input) {
			std::string output;
			for (char c : input) {
				switch (c) {
				case '&':  output += "&amp;";  break;
				case '<':  output += "&lt;";   break;
				case '>':  output += "&gt;";   break;
				case '"':  output += "&quot;"; break;
				case '\'': output += "&#39;";  break;
				default:   output += c;        break;
				}
			}
			return output;
		}

		std::vector<std::string> splitWords(const std::string& text) {
			std::istringstream iss(text);
			std::vector<std::string> words;
			std::string word;
			while (iss >> word) {
				words.push_back(word);
			}
			return words;
		}

		// Reconstruct from words with optional trailing space
		std::string joinWords(const std::vector<std::string>& words) {
			std::string result;
			for (const auto& word : words) {
				result += escapeHTML(word) + " ";
			}
			return result;
		}

		// Compute LCS table
		static std::vector<std::vector<int>> computeLCS(const std::string& a, const std::string& b) {
			size_t n = a.size(), m = b.size();
			std::vector<std::vector<int>> lcs(n + 1, std::vector<int>(m + 1, 0));
			for (size_t i = 0; i < n; ++i) {
				for (size_t j = 0; j < m; ++j) {
					if (a[i] == b[j]) {
						lcs[i + 1][j + 1] = lcs[i][j] + 1;
					}
					else {
						lcs[i + 1][j + 1] = std::max(lcs[i + 1][j], lcs[i][j + 1]);
					}
				}
			}
			return lcs;
		}

		std::vector<std::vector<int>> computeWordLCS(const std::vector<std::string>& a, const std::vector<std::string>& b) {
			size_t n = a.size(), m = b.size();
			std::vector<std::vector<int>> lcs(n + 1, std::vector<int>(m + 1, 0));
			for (size_t i = 0; i < n; ++i) {
				for (size_t j = 0; j < m; ++j) {
					if (a[i] == b[j]) {
						lcs[i + 1][j + 1] = lcs[i][j] + 1;
					}
					else {
						lcs[i + 1][j + 1] = std::max(lcs[i + 1][j], lcs[i][j + 1]);
					}
				}
			}
			return lcs;
		}

		// Backtrack and generate diff with HTML tags and escaping
		std::string stringDiffHTML(const std::string& a, const std::string& b) {
			auto lcs = computeLCS(a, b);
			std::string result;

			size_t i = a.size();
			size_t j = b.size();

			std::string ins, del;

			while (i > 0 || j > 0) {
				if (i > 0 && j > 0 && a[i - 1] == b[j - 1]) {
					if (!ins.empty()) {
						result = "<ins>" + escapeHTML(ins) + "</ins>" + result;
						ins.clear();
					}
					if (!del.empty()) {
						result = "<del>" + escapeHTML(del) + "</del>" + result;
						del.clear();
					}
					result = escapeHTML(std::string(1, a[i - 1])) + result;
					--i;
					--j;
				}
				else if (j > 0 && (i == 0 || lcs[i][j - 1] >= lcs[i - 1][j])) {
					ins = b[j - 1] + ins;
					--j;
				}
				else if (i > 0 && (j == 0 || lcs[i][j - 1] < lcs[i - 1][j])) {
					del = a[i - 1] + del;
					--i;
				}
			}

			if (!ins.empty()) {
				result = "<ins>" + escapeHTML(ins) + "</ins>" + result;
			}
			if (!del.empty()) {
				result = "<del>" + escapeHTML(del) + "</del>" + result;
			}

			return result;
		}

		std::string stringWordDiffHTML(const std::string& a_str, const std::string& b_str) {
			auto a = splitWords(a_str);
			auto b = splitWords(b_str);
			auto lcs = computeWordLCS(a, b);

			size_t i = a.size();
			size_t j = b.size();
			std::vector<std::string> ins, del;
			std::string result;

			// Backtrack from LCS table
			while (i > 0 || j > 0) {
				if (i > 0 && j > 0 && a[i - 1] == b[j - 1]) {
					if (!ins.empty()) {
						result = "<ins>" + joinWords(ins) + "</ins>" + result;
						ins.clear();
					}
					if (!del.empty()) {
						result = "<del>" + joinWords(del) + "</del>" + result;
						del.clear();
					}
					result = escapeHTML(a[i - 1]) + " " + result;
					--i; --j;
				}
				else if (j > 0 && (i == 0 || lcs[i][j - 1] >= lcs[i - 1][j])) {
					ins.insert(ins.begin(), b[j - 1]);
					--j;
				}
				else if (i > 0 && (j == 0 || lcs[i][j - 1] < lcs[i - 1][j])) {
					del.insert(del.begin(), a[i - 1]);
					--i;
				}
			}

			// Flush remaining insertions or deletions
			if (!ins.empty()) {
				result = "<ins>" + joinWords(ins) + "</ins>" + result;
			}
			if (!del.empty()) {
				result = "<del>" + joinWords(del) + "</del>" + result;
			}

			return result;
		}
	}

	namespace proc {

		CommandResult execute(const std::string& command, bool redirectStderrToStdout, const std::string& inputStreamContents) {
			int exitcode = InvalidExitStatus;
			std::array<char, 128> buffer{};
			std::string result;
#ifdef _WIN32
#define popen _popen
#define pclose _pclose
#define WEXITSTATUS
#endif
			std::string cmd = command;
			if (redirectStderrToStdout) {
				cmd += " 2>&1";
			}

			if (!inputStreamContents.empty()) {
				const std::string& inputFileName = "ziran_execute_input_file.txt";
				std::ofstream ofs(inputFileName);
				ofs << inputStreamContents;

				cmd += " <" + inputFileName;
			}

			FILE* pipe = popen(cmd.c_str(), "r");
			if (pipe == nullptr) {
				return CommandResult{ "", exitcode };
			}
			try {
				std::size_t bytesread;
				while ((bytesread = fread(buffer.data(), sizeof(buffer.at(0)), sizeof(buffer), pipe)) != 0) {
					result += std::string(buffer.data(), bytesread);
				}
			}
			catch (...) {
				pclose(pipe);
				throw;
			}

			exitcode = WEXITSTATUS(pclose(pipe));
			return CommandResult{ result, exitcode };
		}
	}
}
