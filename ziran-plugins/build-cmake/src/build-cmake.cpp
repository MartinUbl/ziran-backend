#include "build-cmake.h"
#include "../../../ziran-shared/plugin_utils.h"

#include <queue>
#include <fstream>
#include <sstream>

#include <cstdlib>

namespace
{
	const std::string Compiled_Directory = "compiled";
	const std::string Build_Type_String = "Release";
}

CBuild_CMake_Plugin::CBuild_CMake_Plugin(const filesystem::path& base_path, const ziran::ParamMap& params, ziran::IReporter& reporter, ziran::IEnvironment& env)
	: mBase_Path(base_path), mReporter(reporter), mEnv(env) {

	const char* cmakepath = "";
	mEnv.Get_String("cmake_path", &cmakepath, "");

	mCMake_Path = cmakepath;
	mCMake_Path /= "cmake";
}

std::string CBuild_CMake_Plugin::Extract_Executable_Name(const filesystem::path& cmakelists) {

	std::ifstream ifs(cmakelists);
	if (!ifs.is_open())
		return "";

	const std::string searchedStr = "add_executable(";

	std::string input;
	bool found = false;

	std::string line;
	while (std::getline(ifs, line)) {

		std::string lcase(line);
		std::transform(line.begin(), line.end(), lcase.begin(),
			[](unsigned char c) { return std::tolower(c); });

		auto pos = lcase.find(searchedStr);
		if (pos != std::string::npos) {
			input += lcase.substr(pos);

			int parity = 0;
			do
			{
				parity = 0;
				for (size_t i = 0; i < input.size(); i++) {
					if (input[i] == '(')
						parity++;
					else if (input[i] == ')')
						parity--;
				}

				if (parity == 0) {
					found = true;
					break;
				}

				if (!std::getline(ifs, line)) {
					break;
				}

				input += line;

			} while (parity > 0);

			//
		}
	}

	if (!found)
		return "";

	input = ziran::string::trim(input.substr(searchedStr.size()));
	auto pos = input.find_first_of(' ');
	if (pos == std::string::npos)
		return "";

	return input.substr(0, pos);
}

HRESULT CBuild_CMake_Plugin::Run() {

	filesystem::current_path(mBase_Path);

	// look for the root CMakeLists.txt file, extract the executable name from any given CMakeLists in directory tree

	filesystem::path pathToRootCMake;

	std::vector<std::string> exeName;

	bool match = false;
	{
		std::queue<filesystem::path> traversal;

		traversal.push(mBase_Path);
		while (!match && !traversal.empty())
		{
			auto cur = traversal.front();
			traversal.pop();

			for (auto itr : filesystem::directory_iterator{ cur })
			{
				if (itr.path().filename() == "CMakeLists.txt") {
					if (!match) {
						match = true;
						pathToRootCMake = itr.path();
					}

					auto exe = Extract_Executable_Name(itr.path());
					if (!exe.empty())
						exeName.push_back(exe);
				}

				if (itr.is_directory()) {
					traversal.push(itr.path());
				}
			}
		}
	}

	if (!match) {
		mReporter.Report(ziran::NJob_Report_Type::Error, "Odevzdaný archiv neobsahuje soubor CMakeLists.txt", "build");
		return E_FAIL;
	}

	if (exeName.empty()) {
		mReporter.Report(ziran::NJob_Report_Type::Error, "V zadané adresářové struktuře nebyl nalezen žádný spustitelný CMake cíl.", "build");
		return E_FAIL;
	}

	if (exeName.size() > 1) {
		mReporter.Report(ziran::NJob_Report_Type::Error, ("CMakeLists.txt obsahuje více spustitelných cílů: " + ziran::string::implode(exeName)).c_str(), "build");
		return E_FAIL;
	}

	// create build directories
	auto build_dir = pathToRootCMake.parent_path() / "build";
	filesystem::create_directories(build_dir);

	filesystem::current_path(build_dir);

	ziran::proc::CommandResult res;

	// configure CMake
	res = ziran::proc::execute("cmake .. -DCMAKE_RUNTIME_OUTPUT_DIRECTORY=\"" + Compiled_Directory + "\" -DCMAKE_BUILD_TYPE=\"" + Build_Type_String + "\"", true);
	if (res.exitstatus != 0) {

		// replace all newlines with <br/> for HTML formatting
		auto escOutput = ziran::string::replace_all(res.output, "\r\n", "<br/>");
		escOutput = ziran::string::replace_all(escOutput, "\n", "<br/>");

		const std::string errorMsg = "Konfigurační krok CMake selhal.";
		const std::string htmlFormattedErrorMsg = "CMake konfigurační krok selhal s následujícím výstupem:<br/><br/>" + escOutput;

		mReporter.Report(ziran::NJob_Report_Type::Error, errorMsg.c_str(), "build", htmlFormattedErrorMsg.c_str());
		return E_FAIL;
	}

	// build target
	res = ziran::proc::execute("cmake --build . --config " + Build_Type_String, true);
	if (res.exitstatus != 0) {
		// replace all newlines with <br/> for HTML formatting
		auto escOutput = ziran::string::replace_all(res.output, "\r\n", "<br/>");
		escOutput = ziran::string::replace_all(escOutput, "\n", "<br/>");

		const std::string errorMsg = "Kompilace prostřednictvím nástroje CMake selhala.";
		const std::string htmlFormattedErrorMsg = "Kompilace prostřednictvím nástroje CMake selhala s následujícím výstupem:<br/><br/>" + escOutput;

		mReporter.Report(ziran::NJob_Report_Type::Error, errorMsg.c_str(), "build", htmlFormattedErrorMsg.c_str());
		return E_FAIL;
	}

	// now look for compiled executable file in all standard paths
	// this must consider:
	// current_path/compiled/executable (GNU/Linux, macOS, ... - standard toolchains)
	// current_path/compiled/executable.exe (MS Windows - ninja, make, ...)
	// current_path/compiled/config_name/executable (GNU/Linux, macOS, ... - exotic toolchains)
	// current_path/compiled/config_name/executable.exe (MS Windows - MSVS)

	const std::string exeNameBase = exeName[0];
	filesystem::path finalExePath;
	std::string finalExeName;

	filesystem::current_path(build_dir / Compiled_Directory);

	if (filesystem::exists(exeNameBase) && filesystem::is_regular_file(exeNameBase)) {
		finalExeName = exeNameBase;
		finalExePath = filesystem::current_path() / finalExeName;
	}
	else if (filesystem::exists(exeNameBase + ".exe") && filesystem::is_regular_file(exeNameBase + ".exe")) {
		finalExeName = exeNameBase + ".exe";
		finalExePath = filesystem::current_path() / finalExeName;
	}
	else
	{
		auto specificDir = build_dir / Compiled_Directory / Build_Type_String;

		if (!filesystem::exists(specificDir) || !filesystem::is_directory(specificDir)) {
			mReporter.Report(ziran::NJob_Report_Type::Error, "Výstupní spustitelný soubor nebyl nalezen. Ujistěte se, že v CMakeLists.txt nijak neovlivňujete cestu k výstupním souborům.", "build");
			return E_FAIL;
		}

		filesystem::current_path(specificDir);

		if (filesystem::exists(exeNameBase) && filesystem::is_regular_file(exeNameBase)) {
			finalExeName = exeNameBase;
			finalExePath = filesystem::current_path() / finalExeName;
		}
		else if (filesystem::exists(exeNameBase + ".exe") && filesystem::is_regular_file(exeNameBase + ".exe")) {
			finalExeName = exeNameBase + ".exe";
			finalExePath = filesystem::current_path() / finalExeName;
		}
	}

	mEnv.Set_State_String("output_executable_path", finalExePath.string().c_str());
	mReporter.Report(ziran::NJob_Report_Type::Info, ("Sestavení cíle " + finalExeName + " proběhlo úspěšně").c_str(), "build");

	return S_OK;
}
