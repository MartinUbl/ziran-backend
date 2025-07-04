#include "plugin_utils.h"

#include <algorithm>
#include <array>
#include <fstream>

namespace ziran
{
	ParamMap Map_From_Param_Arrays(const char** keys, const char** values, const size_t sizes)
	{
		if (!keys || !values || sizes == 0)
			return {};

		std::map<std::string, std::string> p;

		for (size_t i = 0; i < sizes; i++)
		{
			if (!keys[i])
				continue;

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

	namespace string
	{
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
				if (i != 0)
					out += delimiter;
				out += vec[i];
			}

			return out;
		}
	}

	namespace proc
	{
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
