#pragma once

#include "../../../ziran-shared/FilesystemLib.h"
#include "../../../ziran-shared/plugin.h"
#include "../../../ziran-shared/plugin_utils.h"

class CBuild_CMake_Plugin {
	private:
		const filesystem::path mBase_Path;
		ziran::IReporter& mReporter;
		ziran::IEnvironment& mEnv;

		filesystem::path mCMake_Path;

	protected:
		std::string Extract_Executable_Name(const filesystem::path& cmakelists);

	public:
		CBuild_CMake_Plugin(const filesystem::path& base_path, const ziran::ParamMap& params, ziran::IReporter& reporter, ziran::IEnvironment& env);

		HRESULT Run();
};
