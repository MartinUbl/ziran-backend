#pragma once

#include "../../../ziran-shared/FilesystemLib.h"
#include "../../../ziran-shared/plugin.h"
#include "../../../ziran-shared/plugin_utils.h"

enum class NMatch_Mode
{
	None,

	Exact,
	Regex,
};

constexpr int Invalid_Exit_Code = -111222333;

class CExec_Output_Match_Plugin
{
	private:
		const filesystem::path mBase_Path;
		NMatch_Mode mMode = NMatch_Mode::None;
		ziran::IReporter& mReporter;
		ziran::IEnvironment& mEnv;
		bool mInclude_Stderr = false;
		bool mShow_Expected = true;
		int mExit_Code = Invalid_Exit_Code;

		std::string mMatch_String;

		std::string mTest_Name = "";
		std::string mParameters = "";
		std::string mStdin_Contents = "";

	public:
		CExec_Output_Match_Plugin(const filesystem::path& base_path, const ziran::ParamMap& params, ziran::IReporter& reporter, ziran::IEnvironment& env);

		HRESULT Run();
};
