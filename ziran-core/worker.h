#pragma once

#include "plugins.h"
#include "database.h"
#include "../ziran-shared/FilesystemLib.h"

#include <thread>
#include <memory>
#include <vector>
#include <mutex>
#include <queue>
#include <condition_variable>

// transfer job from one directory to another, resolves name conflicts if needed
filesystem::path Transfer_Job(const filesystem::path& source, const filesystem::path& destDir);

class CWorker_Pool {
	private:
		CPlugin_Mgr& mPlugin_Mgr;
		CDatabase_Handler& mDatabase;
		const filesystem::path mOut_Dir;

		const size_t mPool_Size;

		std::vector<std::thread> mWorker_Threads;
		std::mutex mQueue_Mutex;
		std::condition_variable mQueue_CondVar;
		std::queue<std::tuple<TJob_Record, filesystem::path, ziran::IEnvironment*>> mJob_Queue;
		bool mTerminate = false;

	private:
		void Worker_Thread(size_t worker_id);

	public:
		CWorker_Pool(CPlugin_Mgr& pluginMgr, CDatabase_Handler& database, const filesystem::path& outDir, size_t pool_size = 1);
		virtual ~CWorker_Pool() = default;

		// runs a job identified by its record on given path
		HRESULT Run_Job(const TJob_Record& jobRec, const filesystem::path& jobPath, ziran::IEnvironment& env);
};
