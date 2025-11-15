#include "../../../ziran-shared/hresult.h"
#include "../../../ziran-shared/plugin.h"

#include "build-cmake.h"

#include <array>

namespace build_cmake {

	constexpr const GUID id = { 0x306aea27, 0xd213, 0x4714, { 0x95, 0xd1, 0x4, 0xa9, 0x0, 0x7f, 0x70, 0xd2 } }; // {306AEA27-D213-4714-95D1-04A9007F70D2}

	ziran::TPlugin_Descriptor desc{
		id,
		"build-cmake",
		0,
		nullptr,
		nullptr,
		nullptr
	};
}

std::array<ziran::TPlugin_Descriptor, 1> descriptors = {
	build_cmake::desc
};

extern "C" HRESULT do_get_plugin_descriptors(ziran::TPlugin_Descriptor** begin, ziran::TPlugin_Descriptor** end) {
	*begin = descriptors.data();
	*end = descriptors.data() + descriptors.size();

	return S_OK;
}

extern "C" HRESULT do_run_plugin(const GUID* id, const char* base_directory, const char** parameter_names, const char** parameter_values, const size_t parameters_count, ziran::IReporter* reporter, ziran::IEnvironment* env) {

	if (*id == build_cmake::id) {
		CBuild_CMake_Plugin plugin(base_directory, ziran::Map_From_Param_Arrays(parameter_names, parameter_values, parameters_count), *reporter, *env);
		return plugin.Run();
	}

	return E_NOTIMPL;
}
