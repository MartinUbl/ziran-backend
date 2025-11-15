#include "../../../ziran-shared/hresult.h"
#include "../../../ziran-shared/plugin.h"

#include "clang-tidy.h"

#include <array>

namespace clang_tidy {
	constexpr const GUID id = { 0xab73ed9b, 0xddf0, 0x4c30, { 0x8d, 0xf4, 0x93, 0xad, 0x22, 0x74, 0x19, 0x3e } }; // {AB73ED9B-DDF0-4C30-8DF4-93AD2274193E}

	const char* const parameter_names[] = {
		"ruleset",
	};

	const char* const parameter_descriptions[] = {
		"Clang-Tidy ruleset to apply (e.g., 'modernize-*', 'cppcoreguidelines-*', 'readability-*', etc.)",
	};

	const ziran::NPlugin_Parameter_Flags parameter_flags[] = {
		ziran::NPlugin_Parameter_Flags::Mandatory,
	};

	ziran::TPlugin_Descriptor desc{
		id,
		"clang-tidy",
		1,
		parameter_names,
		parameter_descriptions,
		parameter_flags
	};
}

std::array<ziran::TPlugin_Descriptor, 1> descriptors = {
	clang_tidy::desc
};

extern "C" HRESULT do_get_plugin_descriptors(ziran::TPlugin_Descriptor** begin, ziran::TPlugin_Descriptor** end) {

	*begin = descriptors.data();
	*end = descriptors.data() + descriptors.size();

	return S_OK;
}

extern "C" HRESULT do_run_plugin(const GUID* id, const char* base_directory, const char** parameter_names, const char** parameter_values, const size_t parameters_count, ziran::IReporter* reporter, ziran::IEnvironment* env) {

	if (*id == clang_tidy::id) {
		CClang_Tidy_Plugin plugin(base_directory, ziran::Map_From_Param_Arrays(parameter_names, parameter_values, parameters_count), *reporter, *env);
		return plugin.Run();
	}

	return E_NOTIMPL;
}
