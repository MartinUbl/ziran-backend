#include "controller.h"

void CDefault_Environment::Set_String(const std::string& key, const std::string& value) {
	mConfig[key] = value;
}

void CDefault_Environment::Set_Input(const std::string& key, const std::string& value) {
	mInputs[key] = value;
}

HRESULT CDefault_Environment::Get_String(const char* key, const char** const target, const char* default_value) const {
	auto valItr = mConfig.find(key);
	if (valItr == mConfig.end()) {
		*target = default_value;
		return S_FALSE;
	}

	*target = valItr->second.c_str();
	return S_OK;
}

HRESULT CDefault_Environment::Get_Input(const char* key, const char** const target) const {
	auto valItr = mInputs.find(key);
	if (valItr == mInputs.end()) {
		return E_FAIL;
	}

	*target = valItr->second.c_str();
	return S_OK;
}

HRESULT CDefault_Environment::Get_State_String(const char* key, const char** const target) const {
	auto valItr = mState_Strings.find(key);
	if (valItr == mState_Strings.end()) {
		return E_NOT_SET;
	}

	*target = valItr->second.c_str();
	return S_OK;
}

HRESULT CDefault_Environment::Set_State_String(const char* key, const char* value) {

	HRESULT res = (mState_Strings.find(key) == mState_Strings.end()) ? S_OK : S_FALSE;

	mState_Strings[key] = value;

	return res;
}

/*
 * Internal reporter class - stores reports to the database
 */
class CReporter : public ziran::IReporter {
	private:
		// stored database reference 
		CDatabase_Handler& mDb;
		// subject job id
		TJob_Id mJob_Id;

	public:
		CReporter(CDatabase_Handler& db, TJob_Id job_id)
			: mDb(db), mJob_Id(job_id) {
			//
		}

		virtual HRESULT Report(ziran::NJob_Report_Type type, const char* str, const char* identifier = nullptr, const char* extendedValue = nullptr) override {
			if (!str) {
				return E_FAIL;
			}

			return mDb.Add_Job_Report(mJob_Id,
				type,
				str,
				identifier ? std::optional<std::string>(identifier) : std::nullopt,
				extendedValue ? std::optional<std::string>(extendedValue) : std::nullopt
			) ? S_OK : E_FAIL;
		}
};

CPipeline_Ctl::CPipeline_Ctl(CDatabase_Handler& db, CPlugin_Mgr& plugins, const TJob_Record& job)
	: mDb(db), mPlugins(plugins), mJob(job), mItems(mDb.Get_Pipeline_Items(mJob.pipeline_id)) {
	//
}

NJob_Output CPipeline_Ctl::Run(const filesystem::path& path, ziran::IEnvironment& env) {
	// reporter instance
	CReporter reporter(mDb, mJob.id);

	// store original work dir
	auto workdir = filesystem::current_path();

	// go through all pipeline items and apply them
	for (const auto& item : mItems) {
		const GUID& plugin_guid = mDb.Get_Plugin_GUID_By_Id(item.plugin_id);

		// run plugin
		HRESULT res = mPlugins.Run_Plugin(plugin_guid, path, item.parameters, reporter, env);

		// return to the original work directory
		filesystem::current_path(workdir);

		// if it returns S_OK (everything OK) or S_FALSE (OK, but no output), carry on; otherwise terminate job with error
		if (!Succeeded(res)) {
			return NJob_Output::Reject;
		}
	}

	return NJob_Output::Accept;
}
