#include "strip.h"

#include <fstream>

CStrip_Plugin::CStrip_Plugin(const filesystem::path& base_path, const ziran::ParamMap& params, ziran::IReporter& reporter)
	: mBase_Path(base_path), mReporter(reporter)
{
	auto itr = params.find("mode");
	if (itr != params.end())
	{
		auto modestr = itr->second;
		if (modestr == "comments")
			mMode = NStrip_Mode::Comments;
		else if (modestr == "whitespace")
			mMode = NStrip_Mode::Excessive_Whitespaces;
	}
}

HRESULT CStrip_Plugin::Run()
{
	if (mMode == NStrip_Mode::None)
	{
		mReporter.Report(ziran::NJob_Report_Type::Error, "No stripping mode specified", "configuration");
		return E_FAIL;
	}

	switch (mMode)
	{
		case NStrip_Mode::Comments:
			return Strip_Comments();
		case NStrip_Mode::Excessive_Whitespaces:
			return Strip_Whitespaces();
		default:
			mReporter.Report(ziran::NJob_Report_Type::Error, "Unhandled stripping mode specified", "configuration");
			return E_INVALIDARG;
	}

	return S_OK;
}


enum class State {
	Normal,
	InString,
	InChar,
	InBlockComment,
	InLineComment
};

std::string removeCommentsFromLine(const std::string& line, State& state) {
	std::string result;
	size_t i = 0;
	while (i < line.size()) {
		char c = line[i];
		char next = (i + 1 < line.size()) ? line[i + 1] : '\0';

		switch (state) {
		case State::Normal:
			if (c == '/' && next == '/') {
				state = State::InLineComment;
				i = line.size();  // skip rest of line
			}
			else if (c == '/' && next == '*') {
				state = State::InBlockComment;
				i += 2;
			}
			else if (c == '"') {
				result += c;
				state = State::InString;
				++i;
			}
			else if (c == '\'') {
				result += c;
				state = State::InChar;
				++i;
			}
			else {
				result += c;
				++i;
			}
			break;

		case State::InString:
			result += c;
			if (c == '\\' && next != '\0') {
				result += next;
				i += 2;
			}
			else if (c == '"') {
				++i;
				state = State::Normal;
			}
			else {
				++i;
			}
			break;

		case State::InChar:
			result += c;
			if (c == '\\' && next != '\0') {
				result += next;
				i += 2;
			}
			else if (c == '\'') {
				++i;
				state = State::Normal;
			}
			else {
				++i;
			}
			break;

		case State::InBlockComment:
			if (c == '*' && next == '/') {
				state = State::Normal;
				i += 2;
			}
			else {
				++i;
			}
			break;

		case State::InLineComment:
			i = line.size();  // skip rest of line
			break;
		}
	}

	if (state == State::InLineComment) {
		state = State::Normal;
	}

	return result;
}

HRESULT CStrip_Plugin::Strip_Comments()
{
	for (auto& entry : filesystem::recursive_directory_iterator(mBase_Path))
	{
		filesystem::path tmp_file = entry.path().string() + "_temp_work_strip_plugin";

		std::ifstream in(entry.path().string());
		if (!in.is_open())
			continue;

		std::ofstream of(tmp_file.string(), std::ios::out | std::ios::trunc);
		if (!of.is_open())
			continue;

		std::string str;
		State state = State::Normal;

		while (std::getline(in, str))
		{
			std::string cleanLine = removeCommentsFromLine(str, state);
			if (!cleanLine.empty() || state == State::Normal)
				of << cleanLine << '\n';
		}
	}

	return S_OK;
}

HRESULT CStrip_Plugin::Strip_Whitespaces()
{
	for (auto& entry : filesystem::recursive_directory_iterator(mBase_Path))
	{
		filesystem::path tmp_file = entry.path().string() + "_temp_work_strip_plugin";

		std::ifstream in(entry.path().string());
		if (!in.is_open())
			continue;

		std::ofstream of(tmp_file.string(), std::ios::out | std::ios::trunc);
		if (!of.is_open())
			continue;

		std::string str;
		while (std::getline(in, str))
		{
			//str.erase(remove_if(str.begin(), str.end(), isspace), str.end()); // TODO
			of << str << '\n';
		}

		in.close();
		of.close();
		filesystem::remove(entry.path()); // Remove original file

		filesystem::rename(tmp_file, entry.path()); // Rename temp file to original
		if (filesystem::exists(tmp_file) && !filesystem::exists(entry.path()))
		{
			const std::string html_msg = "Temporary file exists but original file does not after renaming: " + tmp_file.string();

			mReporter.Report(ziran::NJob_Report_Type::Error, "Failed to rename temporary file to original file", "runtime", html_msg.c_str());
			return E_FAIL;
		}
		else if (!filesystem::exists(tmp_file) && !filesystem::exists(entry.path()))
		{
			const std::string html_msg = "Both original and temporary files do not exist after operation: " + tmp_file.string();

			mReporter.Report(ziran::NJob_Report_Type::Warning, "Both original and temporary files do not exist after operation", "runtime", html_msg.c_str());
		}
	}
	return S_OK;
}
