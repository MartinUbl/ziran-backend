#pragma once

#include <string>
#include <vector>
#include <map>

#include "../ziran-shared/plugin.h"

#include "../ziran-shared/FilesystemLib.h"
#include "../ziran-shared/Dynamic_Library.h"

/*
 * Existing plugin manager
 */
class CPlugin_Mgr {
	private:
		// structure of loaded plugin
		struct TLoaded_Plugin {
			ziran::TPlugin_Descriptor descriptor;
			ziran::TDo_Run_Plugin run;
		};

		// stored plugin directory
		const std::string mPlugin_Directory;

		// vector of loaded libraries (to hold them in memory, and therefore not deallocating all related handles)
		std::vector<CDynamic_Library> mLoaded_Libraries;
		// map of loaded plugins
		std::map<GUID, TLoaded_Plugin> mLoaded_Plugins;

	public:
		CPlugin_Mgr(const std::string& pluginDir);
		virtual ~CPlugin_Mgr() = default;

		// loads plugins from given directory
		bool Load_Plugins();

		// runs plugin on given job entry
		HRESULT Run_Plugin(const GUID& id, const filesystem::path& base_directory, const std::map<std::string, std::string>& parameters, ziran::IReporter& reporter, ziran::IEnvironment& env);
};

