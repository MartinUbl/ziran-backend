#pragma once

#include "../../../ziran-shared/FilesystemLib.h"
#include "../../../ziran-shared/plugin.h"
#include "../../../ziran-shared/plugin_utils.h"

enum class NStrip_Mode {
	None,

	Comments,
	Excessive_Whitespaces,
};

class CStrip_Plugin {
	private:
		const filesystem::path mBase_Path;
		NStrip_Mode mMode = NStrip_Mode::None;
		ziran::IReporter& mReporter;

	protected:
		HRESULT Strip_Comments();
		HRESULT Strip_Whitespaces();

	public:
		CStrip_Plugin(const filesystem::path& base_path, const ziran::ParamMap& params, ziran::IReporter& reporter);

		HRESULT Run();
};
