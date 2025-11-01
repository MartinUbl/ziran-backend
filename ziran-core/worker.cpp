#include "worker.h"
#include "controller.h"

filesystem::path Transfer_Job(const filesystem::path& source, const filesystem::path& destDir)
{
	auto basename = source.filename().string();

	filesystem::path target = destDir / basename;

	int ext = 0;
	while (filesystem::exists(target)) {
		target = destDir / (basename + "_" + std::to_string(++ext));
	}

	filesystem::rename(source, target);

	return target;
}

CWorker_Pool::CWorker_Pool(CPlugin_Mgr& pluginMgr, CDatabase_Handler& database, const filesystem::path& outDir, size_t pool_size)
	: mPlugin_Mgr(pluginMgr), mDatabase(database), mOut_Dir(outDir), mPool_Size(pool_size)
{
	for (size_t i = 0; i < mPool_Size; ++i) {
		mWorker_Threads.emplace_back(&CWorker_Pool::Worker_Thread, this, i);
	}
}

void CWorker_Pool::Worker_Thread(size_t worker_id)
{
	while (true)
	{
		std::tuple<TJob_Record, filesystem::path, ziran::IEnvironment*> job_entry;
		{
			std::unique_lock<std::mutex> lock(mQueue_Mutex);
			mQueue_CondVar.wait(lock, [this]() { return !mJob_Queue.empty() || mTerminate; });
			if (mTerminate && mJob_Queue.empty())
				return;
			job_entry = mJob_Queue.front();
			mJob_Queue.pop();
		}
		const TJob_Record& jobRec = std::get<0>(job_entry);
		const filesystem::path& jobPath = std::get<1>(job_entry);
		ziran::IEnvironment* env = std::get<2>(job_entry);

		mDatabase.Set_Job_Status(jobRec.id, NJob_Status::Work);

		// create pipeline controller
		CPipeline_Ctl ctl(mDatabase, mPlugin_Mgr, jobRec);

		// and perform all tasks
		auto output = ctl.Run(jobPath, *env);

		// then, transfer job to output directory, set status to 'done' and mark output status
		Transfer_Job(jobPath, mOut_Dir);
		mDatabase.Set_Job_Status(jobRec.id, NJob_Status::Done);
		mDatabase.Set_Job_Output(jobRec.id, output);
		mDatabase.Set_Job_Processed_Timestamp(jobRec.id, std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count());

		//std::cout << "Finished " << entry.path().filename() << "!" << std::endl;
	}
}

HRESULT CWorker_Pool::Run_Job(const TJob_Record& jobRec, const filesystem::path& jobPath, ziran::IEnvironment& env)
{
	{
		std::lock_guard<std::mutex> lock(mQueue_Mutex);
		mJob_Queue.emplace(jobRec, jobPath, &env);
	}
	mQueue_CondVar.notify_one();
	return S_OK;
}


