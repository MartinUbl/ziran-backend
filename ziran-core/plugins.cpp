#include "plugins.h"

#include <string>
#include <sstream>
#include <spdlog/spdlog.h>

using namespace std::string_literals;

CPlugin_Mgr::CPlugin_Mgr(const std::string& pluginDir)
	: mPlugin_Directory(pluginDir) {
	//
}

bool CPlugin_Mgr::Load_Plugins(CDatabase_Handler& db) {

	auto updatePluginParameters = [&db](ziran::TPlugin_Descriptor* descriptor) {
		if (descriptor->parameters_count == 0 || descriptor->parameter_names == nullptr || descriptor->parameter_descriptions == nullptr || descriptor->parameter_flags == nullptr) {
			db.Update_Plugin_Parameters_Hint(descriptor->id, "");
			return;
		}

		std::ostringstream paramString;
		for (size_t i = 0; i < descriptor->parameters_count; i++) {
			paramString << descriptor->parameter_names[i] << ":\"" << descriptor->parameter_descriptions[i] << "\"|";
			const auto flags = descriptor->parameter_flags[i];
			if (flags & ziran::NPlugin_Parameter_Flags::Mandatory) {
				paramString << "M";
			}
			else {
				paramString << "0";
			}
			paramString << ";";
		}
		db.Update_Plugin_Parameters_Hint(descriptor->id, paramString.str());
	};

	// go through all files in given directory
	for (auto& libpath : filesystem::directory_iterator(mPlugin_Directory)) {

		// it has to be a regular file
		if (!libpath.is_regular_file()) {
			continue;
		}

		CDynamic_Library lib;
		// check if it's a library file (using extension)
		if (!CDynamic_Library::Is_Library(libpath)) {
			continue;
		}

		// try to load it
		if (!lib.Load(libpath)) {
			continue;
		}

		// try to retrieve descriptor-supplying function
		auto do_get_plugin_descriptors = lib.Resolve<ziran::TDo_Get_Plugin_Descriptors>("do_get_plugin_descriptors");
		if (!do_get_plugin_descriptors) {
			continue;
		}

		// try to retrieve plugin invoker function
		auto do_run_plugin = lib.Resolve<ziran::TDo_Run_Plugin>("do_run_plugin");
		if (!do_run_plugin) {
			continue;
		}

		// attempt to retrieve descriptors
		ziran::TPlugin_Descriptor *begin = nullptr, *end = nullptr;
		if (do_get_plugin_descriptors(&begin, &end) != S_OK) {
			continue;
		}

		// store library so it won't get unloaded (thus making the function addresses invalid)
		mLoaded_Libraries.push_back(std::move(lib));

		// fetch descriptors and store them to internal map
		for (auto* d = begin; d < end; d++) {
			spdlog::info("Loaded plugin: {} from {}", d->name, libpath.path().filename().string());

			updatePluginParameters(d);

			mLoaded_Plugins[d->id] = TLoaded_Plugin{
				*d,
				do_run_plugin
			};
		}
	}

	return true;
}

HRESULT CPlugin_Mgr::Run_Plugin(const GUID& id, const filesystem::path& base_directory, const std::map<std::string, std::string>& parameters, ziran::IReporter& reporter, ziran::IEnvironment& env) {

	// no such plugin
	if (mLoaded_Plugins.find(id) == mLoaded_Plugins.end()) {
		return E_NOTIMPL;
	}

	// copy parameters to two vectors temporarily
	size_t cnt = parameters.size();
	std::vector<const char*> paramNames, paramValues;
	for (const auto& p : parameters) {
		paramNames.push_back(p.first.c_str());
		paramValues.push_back(p.second.c_str());
	}

	// call the plugin invoker
	return mLoaded_Plugins[id].run(&id, base_directory.string().c_str(), paramNames.data(), paramValues.data(), cnt, &reporter, &env);
}
