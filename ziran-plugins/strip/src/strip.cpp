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
		mReporter.Report(ziran::NJob_Report_Type::Error, "No stripping mode specified", "config");
		return E_FAIL;
	}

	switch (mMode)
	{
		case NStrip_Mode::Comments:
			return Strip_Comments();
		case NStrip_Mode::Excessive_Whitespaces:
			return Strip_Whitespaces();
		default:
			mReporter.Report(ziran::NJob_Report_Type::Error, "Unhandled stripping mode specified", "config");
			return E_INVALIDARG;
	}

	return S_OK;
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
		while (std::getline(in, str))
		{
			bool qstate = false, sstate = false;

			size_t i = 0;
			for (i = 0; i < str.size(); i++)
			{
				if (qstate)
				{
					if (str[i] == '"')
						qstate = false;
				}
				else
				{
					if (str[i] == '"')
						qstate = true;
					else if (str[i] == '/')
					{
						//
					}
				}
			}
		}
	}

	return S_OK;
}

HRESULT CStrip_Plugin::Strip_Whitespaces()
{
	return S_OK;
}
