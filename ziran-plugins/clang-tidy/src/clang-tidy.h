#pragma once

#include "../../../ziran-shared/FilesystemLib.h"
#include "../../../ziran-shared/plugin.h"
#include "../../../ziran-shared/plugin_utils.h"

class CClang_Tidy_Plugin {
	private:
		const filesystem::path mBase_Path;
		ziran::IReporter& mReporter;
		ziran::IEnvironment& mEnv;

		filesystem::path mClang_Tidy_Path;
		std::string mClang_Tidy_Rules;

	public:
		CClang_Tidy_Plugin(const filesystem::path& base_path, const ziran::ParamMap& params, ziran::IReporter& reporter, ziran::IEnvironment& env);

		HRESULT Run();
};
