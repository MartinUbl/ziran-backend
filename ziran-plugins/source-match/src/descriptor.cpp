#include "source-match.h"

#include "../../../ziran-shared/hresult.h"
#include "../../../ziran-shared/plugin.h"

#include <array>

namespace source_match
{
	constexpr const GUID id = { 0x94f865d8, 0x4b6d, 0x4b9e, { 0xa5, 0x7a, 0x28, 0xf5, 0x53, 0x18, 0x12, 0x4 } };; // {94F865D8-4B6D-4B9E-A57A-28F553181204}

	ziran::TPlugin_Descriptor desc{
		id,
		"source-match"
	};
}

std::array<ziran::TPlugin_Descriptor, 1> descriptors = {
	source_match::desc
};

extern "C" HRESULT do_get_plugin_descriptors(ziran::TPlugin_Descriptor** begin, ziran::TPlugin_Descriptor** end)
{
	*begin = descriptors.data();
	*end = descriptors.data() + descriptors.size();

	return S_OK;
}

extern "C" HRESULT do_run_plugin(const GUID* id, const char* base_directory, const char** parameter_names, const char** parameter_values, const size_t parameters_count, ziran::IReporter *reporter, ziran::IEnvironment* env)
{
	if (*id == source_match::id)
	{
		CSource_Match_Plugin plugin(base_directory, ziran::Map_From_Param_Arrays(parameter_names, parameter_values, parameters_count), *reporter, *env);
		return plugin.Run();
	}

	return E_NOTIMPL;
}
