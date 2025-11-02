#pragma once

#include "database.h"
#include "plugins.h"
#include "../ziran-shared/plugin.h"

class CDefault_Environment : public ziran::IEnvironment {
	private:
		std::map<std::string, std::string> mConfig;
		std::map<std::string, std::string> mInputs;
		std::map<std::string, std::string> mState_Strings;

	public:
		void Set_String(const std::string& key, const std::string& value);
		void Set_Input(const std::string& key, const std::string& value);

		virtual HRESULT Get_String(const char* key, const char** const target, const char* default_value = nullptr) const override;
		virtual HRESULT Get_Input(const char* key, const char** const target) const override;

		virtual HRESULT Get_State_String(const char* key, const char** const target) const override;
		virtual HRESULT Set_State_String(const char* key, const char* value) override;
};

/*
 * Pipeline controller - processes the pipeline on given job
 */
class CPipeline_Ctl {
	private:
		// database reference
		CDatabase_Handler& mDb;

		// plugin manafer reference
		CPlugin_Mgr& mPlugins;

		// stored job record
		const TJob_Record& mJob;

		// stored pipeline items to be processed
		const std::vector<TPipeline_Item_Record>& mItems;

	public:
		CPipeline_Ctl(CDatabase_Handler& db, CPlugin_Mgr& plugins, const TJob_Record& job);

		// run the job pipeline, return success indicator
		NJob_Output Run(const filesystem::path& path, ziran::IEnvironment& env);
};
