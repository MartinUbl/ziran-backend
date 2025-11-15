#include "execmatch.h"

#include "../../../ziran-shared/hresult.h"
#include "../../../ziran-shared/plugin.h"

#include <array>

namespace execmatch {
	constexpr const GUID id = { 0xe33532e5, 0x99bb, 0x4aa0, { 0x89, 0xd0, 0xfb, 0x8, 0x50, 0x1a, 0x37, 0x3a } };	// {E33532E5-99BB-4AA0-89D0-FB08501A373A}

	const char* const parameter_names[] = {
		"name",
		"match",
		"string",
		"parameters",
		"include_stderr"
	};

	const char* const parameter_descriptions[] = {
		"Name of the check",
		"Expected output match (exact, regex)",
		"Matching string or regex",
		"Additional parameters to pass to the executable (space-separated)",
		"Whether to include stderr output in the report (true/false)"
	};

	const ziran::NPlugin_Parameter_Flags parameter_flags[] = {
		ziran::NPlugin_Parameter_Flags::Mandatory,
		ziran::NPlugin_Parameter_Flags::Mandatory,
		ziran::NPlugin_Parameter_Flags::Mandatory,
		ziran::NPlugin_Parameter_Flags::None,
		ziran::NPlugin_Parameter_Flags::None
	};

	ziran::TPlugin_Descriptor desc{
		id,
		"exec-output-match",
		5,
		parameter_names,
		parameter_descriptions,
		parameter_flags
	};
}

std::array<ziran::TPlugin_Descriptor, 1> descriptors = {
	execmatch::desc
};

extern "C" HRESULT do_get_plugin_descriptors(ziran::TPlugin_Descriptor** begin, ziran::TPlugin_Descriptor** end) {

	*begin = descriptors.data();
	*end = descriptors.data() + descriptors.size();

	return S_OK;
}

extern "C" HRESULT do_run_plugin(const GUID* id, const char* base_directory, const char** parameter_names, const char** parameter_values, const size_t parameters_count, ziran::IReporter *reporter, ziran::IEnvironment* env) {

	if (*id == execmatch::id) {
		CExec_Output_Match_Plugin plugin(base_directory, ziran::Map_From_Param_Arrays(parameter_names, parameter_values, parameters_count), *reporter, *env);
		return plugin.Run();
	}

	return E_NOTIMPL;
}
