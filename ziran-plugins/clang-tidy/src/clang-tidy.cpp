#include "clang-tidy.h"
#include "../../../ziran-shared/plugin_utils.h"

#include <queue>
#include <fstream>
#include <sstream>

#include <cstdlib>

CClang_Tidy_Plugin::CClang_Tidy_Plugin(const filesystem::path& base_path, const ziran::ParamMap& params, ziran::IReporter& reporter, ziran::IEnvironment& env)
	: mBase_Path(base_path), mReporter(reporter), mEnv(env) {

	const char* clangtidypath = "";
	if (Succeeded(mEnv.Get_String("clang_tidy_path", &clangtidypath, ""))) {
		mClang_Tidy_Path = clangtidypath;

		if (filesystem::exists(mClang_Tidy_Path / "clang-tidy")) {
			mClang_Tidy_Path /= "clang-tidy";
		}
		else if (filesystem::exists(mClang_Tidy_Path / "clang-tidy.exe")) {
			mClang_Tidy_Path /= "clang-tidy.exe";
		}
		else {
			mReporter.Report(ziran::NJob_Report_Type::Error, "Invalid clang-tidy path configured", "configuration");
			return;
		}

		std::string str;
		if (ziran::Param_Get(params, "ruleset", str)) {
			mClang_Tidy_Rules = ziran::string::trim_whitespaces(str);
		}
	}
}

HRESULT CClang_Tidy_Plugin::Run() {

	filesystem::current_path(mBase_Path);

	// traverse the directory tree to find all source files (cpp, c, h, hpp, ixx, cppm)
	std::vector<filesystem::path> sourceFiles;
	std::queue<filesystem::path> traversalQueue;
	traversalQueue.push(mBase_Path);
	while (!traversalQueue.empty()) {
		auto currentPath = traversalQueue.front();
		traversalQueue.pop();
		for (const auto& entry : filesystem::directory_iterator(currentPath)) {
			if (entry.is_directory()) {
				traversalQueue.push(entry.path());
			}
			else if (entry.is_regular_file()) {
				auto ext = entry.path().extension().string();
				if (ext == ".cpp" || ext == ".c" || ext == ".h" || ext == ".hpp" || ext == ".ixx" || ext == ".cppm") {
					sourceFiles.push_back(entry.path());
				}
			}
		}
	}

	// run clang-tidy on each source file
	for (const auto& sourceFile : sourceFiles) {
		std::string command = mClang_Tidy_Path.string() + " -checks=\"" + mClang_Tidy_Rules + "\"  --warnings-as-errors=* \"" + sourceFile.string() + "\"";
		ziran::proc::CommandResult res = ziran::proc::execute(command, true);
		if (res.exitstatus != 0) {
			// replace all newlines with <br/> for HTML formatting
			auto escOutput = ziran::string::replace_all(res.output, "\r\n", "<br/>");
			escOutput = ziran::string::replace_all(escOutput, "\n", "<br/>");
			const std::string errorMsg = "Spuštění nástroje clang-tidy selhalo pro soubor: " + sourceFile.string();
			const std::string htmlFormattedErrorMsg = "Nástroj clang-tidy selhal s následujícím výstupem:<br/><br/>" + escOutput;
			mReporter.Report(ziran::NJob_Report_Type::Error, errorMsg.c_str(), "clang-tidy", htmlFormattedErrorMsg.c_str());
			return S_OK;
		}
	}

	mReporter.Report(ziran::NJob_Report_Type::Info, "Task clang-tidy proběhl v pořádku", "clang-tidy");

	return S_OK;
}
