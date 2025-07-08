#include <iostream>
#include <vector>

#include "config.h"
#include "plugins.h"
#include "database.h"
#include "controller.h"
#include "jobmgr.h"
#include "SimpleIni.h"
#include <functional>

#include "../ziran-shared/FilesystemLib.h"

#include <thread>
#include <chrono>

// transparent routine caller, passes log vector and generalizes the procedure
template<typename TFunc, typename... FncArgs>
auto Startup_Routine(const std::string& what, TFunc func, FncArgs... args)
{
	std::cout << std::endl << what;

	std::vector<std::string> vec;
	auto res = func(vec, args...);

	std::cout << (res ? " OK" : " Fail") << std::endl;
	if (!vec.empty())
	{
		for (const auto& err : vec)
			std::cout << " - " << err << std::endl;
	}

	return res;
}

// transfer job from one directory to another, resolves name conflicts if needed
filesystem::path Transfer_Job(const filesystem::path& source, const filesystem::path& destDir)
{
	auto basename = source.filename().string();

	filesystem::path target = destDir / basename;

	int ext = 0;
	while (filesystem::exists(target))
		target = destDir / (basename + "_" + std::to_string(++ext));

	filesystem::rename(source, target);

	return target;
}

int main(int argc, char** argv)
{
	std::cout	<< "Ziran - semestral work validator" << std::endl
				<< "--------------------------------" << std::endl;

	TConfig cfg;
	CPlugin_Mgr mgr(Get_Application_Dir().string());
	CDatabase_Handler db;
	CDefault_Environment env;
	CJob_Mgr jobMgr(cfg);

	// at first, load configuration file if any; otherwise use defaults
	if (!Startup_Routine("Loading configuration...", [&cfg](std::vector<std::string>& log) -> bool {

		CSimpleIni conf;
		if (conf.LoadFile((Get_Application_Dir() / "ziran.ini").string().c_str()) == SI_Error::SI_OK)
		{
			cfg.inDir = conf.GetValue("core", "input_dir", cfg.inDir.string().c_str());
			cfg.outDir = conf.GetValue("core", "output_dir", cfg.outDir.string().c_str());
			cfg.workDir = conf.GetValue("core", "work_dir", cfg.workDir.string().c_str());
			cfg.discardDir = conf.GetValue("core", "discard_dir", cfg.discardDir.string().c_str());

			cfg.bindIpString = conf.GetValue("listener", "bind_ip", cfg.bindIpString.c_str());
			long bindport = conf.GetLongValue("listener", "port", cfg.bindPort);
			if (bindport < 0 || bindport > 65535)
				log.push_back("invalid bind port used, falling back to default");
			else
				cfg.bindPort = static_cast<uint16_t>(bindport);

			// TODO: validate directories

			cfg.dbHost = conf.GetValue("database", "host", cfg.dbHost.c_str());
			cfg.dbPort = 3306;
			auto port = conf.GetLongValue("database", "port", cfg.dbPort);
			if (port < 0 || port > 65535)
				log.push_back("invalid database port used, falling back to default");
			else
				cfg.dbPort = static_cast<uint16_t>(port);

			cfg.dbUser = conf.GetValue("database", "username", cfg.dbUser.c_str());
			cfg.dbPassword = conf.GetValue("database", "password", cfg.dbPassword.c_str());
			cfg.dbName = conf.GetValue("database", "dbname", cfg.dbName.c_str());
		}
		else
			log.push_back("No configuration file found.");

		return true;

	})) return 1;

	// then load plugins
	if (!Startup_Routine("Loading plugins...", std::bind(&CPlugin_Mgr::Load_Plugins, &mgr, std::placeholders::_1)))
		return 2;

	// connect to database
	if (!Startup_Routine("Connecting to database...", std::bind(&CDatabase_Handler::Connect, &db, cfg.dbHost, cfg.dbPort, cfg.dbUser, cfg.dbPassword, cfg.dbName, std::placeholders::_1)))
		return 3;

	// prepare statements to be executed
	if (!Startup_Routine("Preparing statements...", std::bind(&CDatabase_Handler::Init_Statements, &db, std::placeholders::_1)))
		return 3;

	// load DB global config
	if (!Startup_Routine("Loading database global config...", std::bind(&CDatabase_Handler::Load_DB_Global_Config, &db, std::placeholders::_1, std::ref(env))))
		return 4;

	// load DB inputs
	if (!Startup_Routine("Loading database inputs...", std::bind(&CDatabase_Handler::Load_DB_Inputs, &db, std::placeholders::_1, std::ref(env))))
		return 4;

	// load DB-stored plugins
	if (!Startup_Routine("Loading database plugins...", std::bind(&CDatabase_Handler::Load_DB_Plugins, &db, std::placeholders::_1)))
		return 4;

	// load pipelines
	if (!Startup_Routine("Loading database pipelines...", std::bind(&CDatabase_Handler::Load_DB_Pipelines, &db, std::placeholders::_1)))
		return 4;

	// load pipeline items
	if (!Startup_Routine("Loading database pipeline items...", std::bind(&CDatabase_Handler::Load_DB_Pipeline_Items, &db, std::placeholders::_1)))
		return 4;

	// start job manager
	if (!Startup_Routine("Starting up job wakeup listener...", std::bind(&CJob_Mgr::Start, &jobMgr, std::placeholders::_1)))
		return 5;

	std::cout << std::endl << "Entering main loop." << std::endl << std::endl;

	// append application directory to relative paths
	if (cfg.inDir.is_relative())
		cfg.inDir = Get_Application_Dir() / cfg.inDir;
	if (cfg.outDir.is_relative())
		cfg.outDir = Get_Application_Dir() / cfg.outDir;
	if (cfg.workDir.is_relative())
		cfg.workDir = Get_Application_Dir() / cfg.workDir;
	if (cfg.discardDir.is_relative())
		cfg.discardDir = Get_Application_Dir() / cfg.discardDir;

	// create directories if not exist
	if (!filesystem::exists(cfg.inDir))
		filesystem::create_directories(cfg.inDir);
	if (!filesystem::exists(cfg.outDir))
		filesystem::create_directories(cfg.outDir);
	if (!filesystem::exists(cfg.workDir))
		filesystem::create_directories(cfg.workDir);
	if (!filesystem::exists(cfg.discardDir))
		filesystem::create_directories(cfg.discardDir);

	// if the work directory is not empty, it means the daemon crashed and there are jobs left to process; move them back to input directory
	for (auto& entry : filesystem::directory_iterator(cfg.workDir)) {
		if (entry.is_directory() && filesystem::exists(entry.path() / "job-meta"))
		{
			std::cout << "Restoring " << entry.path().filename() << " to input directory..." << std::endl;
			Transfer_Job(entry.path(), cfg.inDir);
		}
		else
		{
			std::cout << "Discarding " << entry.path().filename() << " (invalid job entry)..." << std::endl;
			Transfer_Job(entry.path(), cfg.discardDir);
		}
	}

	while (true)
	{
		// at first, go through the input directory to scan for job left from the time the daemon was down
		// then, await another input

		// go thorugh input directory
		for (auto& entry : filesystem::directory_iterator(cfg.inDir))
		{
			// it must be a directory containing job-meta file
			if (entry.is_directory() && filesystem::exists(entry.path() / "job-meta"))
			{
				std::cout << "Processing " << entry.path().filename() << "..." << std::endl;

				// it must be present in database by its unique name (frontend ensures that)
				TJob_Record rec = db.Get_Job_By_Name(entry.path().filename().string());
				if (rec.id == Invalid_Job_Id)
				{
					std::cout << "Discarding " << entry.path().filename() << " (job not in database)..." << std::endl;
					Transfer_Job(entry.path(), cfg.discardDir);
				}
				// it must also not be in "Done" state, that would indicate an error during processing
				else if (rec.status == NJob_Status::Done)
				{
					std::cout << "Discarding " << entry.path().filename() << " (job has invalid state)..." << std::endl;
					Transfer_Job(entry.path(), cfg.discardDir);
				}
				else
				{
					// transfer job to work directory and change status
					const filesystem::path work_dest = Transfer_Job(entry.path(), cfg.workDir);
					db.Set_Job_Status(rec.id, NJob_Status::Work);

					// create pipeline controller
					CPipeline_Ctl ctl(db, mgr, rec);

					// and perform all tasks
					auto output = ctl.Run(work_dest, env);

					// then, transfer job to output directory, set status to 'done' and mark output status
					Transfer_Job(work_dest, cfg.outDir);
					db.Set_Job_Status(rec.id, NJob_Status::Done);
					db.Set_Job_Output(rec.id, output);
					db.Set_Job_Processed_Timestamp(rec.id, std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count());

					std::cout << "Finished " << entry.path().filename() << "!" << std::endl;
				}
			}
			else
			{
				std::cout << "Discarding " << entry.path().filename() << " (invalid job entry)..." << std::endl;
				Transfer_Job(entry.path(), cfg.discardDir);
			}
		}

		// await an input
		auto reason = jobMgr.Await();

		// if the wakeup reason is "none", it means the jobMgr encountered an error or was instructed to terminate
		if (reason == NWakeUp_Reason::None) {
			std::cout << "Terminating main loop..." << std::endl;
			break;
		}

		// reload = command by the frontend to reload cached items
		if (reason == NWakeUp_Reason::Reload) {
			// TODO: reload pipelines and items
			continue;
		}
	}

	jobMgr.Stop();

	return 0;
}
