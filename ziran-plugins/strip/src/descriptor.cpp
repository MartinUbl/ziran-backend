#include "strip.h"

#include "../../../ziran-shared/hresult.h"
#include "../../../ziran-shared/plugin.h"

#include <array>

namespace strip
{
	constexpr const GUID id = { 0xfbd1952d, 0x4cbc, 0x4387, { 0x9e, 0xbb, 0x9f, 0x16, 0x9c, 0xae, 0xec, 0x63 } }; // {FBD1952D-4CBC-4387-9EBB-9F169CAEEC63}

	ziran::TPlugin_Descriptor desc{
		id,
		"strip"
	};
}

std::array<ziran::TPlugin_Descriptor, 1> descriptors = {
	strip::desc
};

extern "C" HRESULT do_get_plugin_descriptors(ziran::TPlugin_Descriptor** begin, ziran::TPlugin_Descriptor** end)
{
	*begin = descriptors.data();
	*end = descriptors.data() + descriptors.size();

	return S_OK;
}

extern "C" HRESULT do_run_plugin(const GUID* id, const char* base_directory, const char** parameter_names, const char** parameter_values, const size_t parameters_count, ziran::IReporter *reporter, ziran::IEnvironment* env)
{
	if (*id == strip::id)
	{
		CStrip_Plugin plugin(base_directory, ziran::Map_From_Param_Arrays(parameter_names, parameter_values, parameters_count), *reporter);
		return plugin.Run();
	}

	return E_NOTIMPL;
}
