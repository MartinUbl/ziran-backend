#include <iostream>
#include <vector>

#include "config.h"
#include "plugins.h"
#include "database.h"
#include "controller.h"
#include "jobmgr.h"
#include "worker.h"
#include "watchdog.h"
#include "SimpleIni.h"
#include <functional>

#include "../ziran-shared/FilesystemLib.h"

#include <thread>
#include <chrono>

#include <spdlog/spdlog.h>

// transparent routine caller, passes log vector and generalizes the procedure
template<typename TFunc, typename... FncArgs>
auto Startup_Routine(const std::string& what, TFunc func, FncArgs... args) {
	spdlog::info("Step {}", what);

	auto res = func(args...);

	if (res) {
		spdlog::info("Step {} completed successfully", what);
	}
	else {
		spdlog::error("Step {} failed", what);
	}

	return res;
}

int main(int argc, char** argv) {

	spdlog::info("Ziran daemon starting up...");

	TConfig cfg;
	CPlugin_Mgr mgr(Get_Application_Dir().string());
	CDatabase_Handler db;
	CDefault_Environment env;
	CJob_Mgr jobMgr(cfg);

	// at first, load configuration file if any; otherwise use defaults
	if (!Startup_Routine("config/load", [&cfg]() -> bool {

		CSimpleIni conf;
		if (conf.LoadFile((Get_Application_Dir() / "ziran.ini").string().c_str()) == SI_Error::SI_OK) {
			cfg.inDir = conf.GetValue("core", "input_dir", cfg.inDir.string().c_str());
			cfg.outDir = conf.GetValue("core", "output_dir", cfg.outDir.string().c_str());
			cfg.workDir = conf.GetValue("core", "work_dir", cfg.workDir.string().c_str());
			cfg.discardDir = conf.GetValue("core", "discard_dir", cfg.discardDir.string().c_str());
			cfg.watchdogFile = conf.GetValue("core", "watchdog_file", cfg.watchdogFile.c_str());

			cfg.bindIpString = conf.GetValue("listener", "bind_ip", cfg.bindIpString.c_str());
			long bindport = conf.GetLongValue("listener", "port", cfg.bindPort);
			if (bindport < 0 || bindport > 65535) {
				spdlog::warn("invalid bind port used, falling back to default");
			}
			else {
				cfg.bindPort = static_cast<uint16_t>(bindport);
			}

			// TODO: validate directories

			cfg.dbHost = conf.GetValue("database", "host", cfg.dbHost.c_str());
			cfg.dbPort = 3306;
			auto port = conf.GetLongValue("database", "port", cfg.dbPort);
			if (port < 0 || port > 65535) {
				spdlog::warn("invalid database port used, falling back to default");
			}
			else {
				cfg.dbPort = static_cast<uint16_t>(port);
			}

			cfg.dbUser = conf.GetValue("database", "username", cfg.dbUser.c_str());
			cfg.dbPassword = conf.GetValue("database", "password", cfg.dbPassword.c_str());
			cfg.dbName = conf.GetValue("database", "dbname", cfg.dbName.c_str());
		}
		else {
			spdlog::warn("No configuration file found, using defaults.");
		}

		return true;

	})) {
		return 1;
	}

	// then load plugins
	if (!Startup_Routine("plugins/load", std::bind(&CPlugin_Mgr::Load_Plugins, &mgr))) {
		return 2;
	}

	// connect to database
	if (!Startup_Routine("database/connect", std::bind(&CDatabase_Handler::Connect, &db, cfg.dbHost, cfg.dbPort, cfg.dbUser, cfg.dbPassword, cfg.dbName))) {
		return 3;
	}

	// prepare statements to be executed
	if (!Startup_Routine("database/statements", std::bind(&CDatabase_Handler::Init_Statements, &db))) {
		return 3;
	}

	// load DB global config
	if (!Startup_Routine("database/global_config", std::bind(&CDatabase_Handler::Load_DB_Global_Config, &db, std::ref(env)))) {
		return 4;
	}

	// load DB inputs
	if (!Startup_Routine("database/inputs", std::bind(&CDatabase_Handler::Load_DB_Inputs, &db, std::ref(env)))) {
		return 4;
	}

	// load DB-stored plugins
	if (!Startup_Routine("database/plugins", std::bind(&CDatabase_Handler::Load_DB_Plugins, &db))) {
		return 4;
	}

	// load pipelines
	if (!Startup_Routine("database/pipelines", std::bind(&CDatabase_Handler::Load_DB_Pipelines, &db))) {
		return 4;
	}

	// load pipeline items
	if (!Startup_Routine("database/pipeline_items", std::bind(&CDatabase_Handler::Load_DB_Pipeline_Items, &db))) {
		return 4;
	}

	// start job manager
	if (!Startup_Routine("job_manager/start", std::bind(&CJob_Mgr::Start, &jobMgr))) {
		return 5;
	}

	spdlog::info("Entering main loop...");

	// append application directory to relative paths
	if (cfg.inDir.is_relative()) {
		cfg.inDir = Get_Application_Dir() / cfg.inDir;
	}
	if (cfg.outDir.is_relative()) {
		cfg.outDir = Get_Application_Dir() / cfg.outDir;
	}
	if (cfg.workDir.is_relative()) {
		cfg.workDir = Get_Application_Dir() / cfg.workDir;
	}
	if (cfg.discardDir.is_relative()) {
		cfg.discardDir = Get_Application_Dir() / cfg.discardDir;
	}

	// create directories if not exist
	if (!filesystem::exists(cfg.inDir)) {
		filesystem::create_directories(cfg.inDir);
	}
	if (!filesystem::exists(cfg.outDir)) {
		filesystem::create_directories(cfg.outDir);
	}
	if (!filesystem::exists(cfg.workDir)) {
		filesystem::create_directories(cfg.workDir);
	}
	if (!filesystem::exists(cfg.discardDir)) {
		filesystem::create_directories(cfg.discardDir);
	}

	// if the work directory is not empty, it means the daemon crashed and there are jobs left to process; move them back to input directory
	for (auto& entry : filesystem::directory_iterator(cfg.workDir)) {
		if (entry.is_directory() && filesystem::exists(entry.path() / "job-meta")) {
			spdlog::warn("Restoring {} to input directory...", entry.path().filename().string());
			Transfer_Job(entry.path(), cfg.inDir);
		}
		else {
			spdlog::warn("Discarding invalid job entry {}...", entry.path().filename().string());
			Transfer_Job(entry.path(), cfg.discardDir);
		}
	}

	spdlog::info("Starting worker pool...");

	CWorker_Pool workerPool(mgr, db, cfg.outDir, 1);// std::thread::hardware_concurrency());

	// start watchdog
	spdlog::info("Starting watchdog...");
	CWatchdog::Get_Instance().Start((Get_Application_Dir() / cfg.watchdogFile).string());

	while (true) {
		CWatchdog::Get_Instance().Kick(NWatchdog_Source::Periodic_Check);

		// at first, go through the input directory to scan for job left from the time the daemon was down
		// then, await another input

		// go thorugh input directory
		for (auto& entry : filesystem::directory_iterator(cfg.inDir)) {
			// it must be a directory containing job-meta file
			if (entry.is_directory() && filesystem::exists(entry.path() / "job-meta")) {
				spdlog::info("Found job entry {} in input directory.", entry.path().filename().string());

				// it must be present in database by its unique name (frontend ensures that)
				TJob_Record rec = db.Get_Job_By_Name(entry.path().filename().string());
				if (rec.id == Invalid_Job_Id) {
					spdlog::warn("Discarding {} (job not in database)...", entry.path().filename().string());
					Transfer_Job(entry.path(), cfg.discardDir);
				}
				// it must also not be in "Done" state, that would indicate an error during processing
				else if (rec.status == NJob_Status::Done) {
					spdlog::warn("Discarding {} (job has invalid state)...", entry.path().filename().string());
					Transfer_Job(entry.path(), cfg.discardDir);
				}
				else {
					spdlog::info("Submitting {} to processing...", entry.path().filename().string());

					// transfer job to work directory and change status
					const filesystem::path work_dest = Transfer_Job(entry.path(), cfg.workDir);

					workerPool.Run_Job(rec, work_dest, env);
				}
			}
			else {
				spdlog::warn("Discarding invalid job entry {}...", entry.path().filename().string());
				Transfer_Job(entry.path(), cfg.discardDir);
			}
		}

		// await an input
		auto reason = jobMgr.Await(std::chrono::milliseconds(2500));

		if (reason == NWakeUp_Reason::Timeout) {
			// timeout, just continue
			continue;
		}

		// if the wakeup reason is "none", it means the jobMgr encountered an error or was instructed to terminate
		if (reason == NWakeUp_Reason::None) {
			spdlog::info("Job manager requested shutdown, exiting main loop...");
			break;
		}

		// reload = command by the frontend to reload cached items
		if (reason == NWakeUp_Reason::Reload) {
			// TODO: reload pipelines and items
			continue;
		}
	}

	CWatchdog::Get_Instance().Stop();

	jobMgr.Stop();

	return 0;
}
