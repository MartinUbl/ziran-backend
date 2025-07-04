#include "execmatch.h"

#include <regex>

CExec_Output_Match_Plugin::CExec_Output_Match_Plugin(const filesystem::path& base_path, const ziran::ParamMap& params, ziran::IReporter& reporter, ziran::IEnvironment& env) 
	: mBase_Path(base_path), mReporter(reporter), mEnv(env) {

	std::string str;
	if (ziran::Param_Get(params, "match", str)) {
		if (str == "exact")
			mMode = NMatch_Mode::Exact;
		else if (str == "regex")
			mMode = NMatch_Mode::Regex;
	}

	if (ziran::Param_Get(params, "string", str)) {
		mMatch_String = ziran::string::trim_whitespaces(str);
	}

	if (ziran::Param_Get(params, "exitcode", str)) {
		try {
			mExit_Code = std::stol(str);
		}
		catch (...) {
			mReporter.Report(ziran::NJob_Report_Type::Error, "Invalid expected exit code for execute output match plugin", "configuration");
		}
	}

	if (ziran::Param_Get(params, "include_stderr", str)) {
		mInclude_Stderr = (str == "true" || str == "yes" || str == "1");
	}

	if (ziran::Param_Get(params, "show_expected", str)) {
		mShow_Expected = (str == "true" || str == "yes" || str == "1");
	}

	if (ziran::Param_Get(params, "name", str)) {
		mTest_Name = str;
	}

	if (ziran::Param_Get(params, "parameters", str)) {
		if (str.size() > 128) {
			mReporter.Report(ziran::NJob_Report_Type::Error, "Too long parameters configuration string", "configuration");
		}
		else {
			mParameters = str;
		}
	}

	if (ziran::Param_Get(params, "stdin", str)) {
		mStdin_Contents = str;
	}
}

HRESULT CExec_Output_Match_Plugin::Run()
{
	if (mMode == NMatch_Mode::None) {
		mReporter.Report(ziran::NJob_Report_Type::Error, "No valid mode configured for execute output match plugin", "configuration");
		return E_FAIL;
	}

	if (mMatch_String.empty()) {
		mReporter.Report(ziran::NJob_Report_Type::Error, "No validation string configured for execute output match plugin", "configuration");
		return E_FAIL;
	}

	const char* execName = nullptr;
	if (mEnv.Get_State_String("output_executable_path", &execName) != S_OK || execName == nullptr || strlen(execName) == 0) {
		mReporter.Report(ziran::NJob_Report_Type::Error, "Invalid configuration - exec output match plugin requires a output_executable_path variable to be set to point to a valid executable file", "configuration");
		return E_FAIL;
	}

	ziran::proc::CommandResult res;

	res = ziran::proc::execute(std::string(execName) + " " + mParameters, mInclude_Stderr, mStdin_Contents);
	if (mExit_Code != Invalid_Exit_Code && mExit_Code != res.exitstatus) {
		mReporter.Report(ziran::NJob_Report_Type::Error, ("Program exited with status code " + std::to_string(res.exitstatus) + (mShow_Expected ? ", expected " + std::to_string(mExit_Code) : "")).c_str(), "output");
		return E_FAIL;
	}

	auto actualOutput = ziran::string::trim_whitespaces(res.output);

	if (mMode == NMatch_Mode::Exact)
	{
		if (actualOutput != mMatch_String) {
			mReporter.Report(ziran::NJob_Report_Type::Error, ("Program output does not match. Actual output:\n " + actualOutput + (mShow_Expected ? "\n\n, expected:\n" + mMatch_String : "")).c_str(), "output");
			return E_FAIL;
		}
	}
	else if (mMode == NMatch_Mode::Regex)
	{
		std::regex r{ mMatch_String };
		std::smatch match;

		if (!std::regex_match(actualOutput, match, r)) {
			mReporter.Report(ziran::NJob_Report_Type::Error, ("Program output does not match. Actual output:\n " + actualOutput + (mShow_Expected ? "\n\n, expected regular expression match to:\n" + mMatch_String : "")).c_str(), "output");
			return E_FAIL;
		}
	}

	if (mTest_Name.empty()) {
		mReporter.Report(ziran::NJob_Report_Type::Info, "Output validation OK", "output");
	}
	else {
		mReporter.Report(ziran::NJob_Report_Type::Info, ("Output validation '" + mTest_Name  +"' OK").c_str(), "output");
	}

	return S_OK;
}
