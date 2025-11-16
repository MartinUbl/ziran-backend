#pragma once

#include <chrono>
#include <map>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <fstream>

class CWorker_Pool;

constexpr size_t Watchdog_Check_Interval_MS = 2000;

constexpr size_t Watchdog_Kick_Timeout_MS = 6500;

enum class NWatchdog_Source {
	Periodic_Check,
	Job_Manager,

	start = Periodic_Check,
	end = Job_Manager,
	count = static_cast<size_t>(NWatchdog_Source::end) - static_cast<size_t>(NWatchdog_Source::start) + 1
};

enum class NWatchdog_Status {
	Inactive,
	Operational,
	Expired,
	Terminated
};

class CWatchdog final {
	private:
		static CWatchdog sInstance;

	private:
		NWatchdog_Status mStatus = NWatchdog_Status::Inactive;
		std::unique_ptr<std::thread> mWD_Thread;
		std::mutex mMtx;
		std::condition_variable mCv;

		bool mRunning = false;

		std::ofstream mWatchdog_File;

		std::map<NWatchdog_Source, std::chrono::steady_clock::time_point> mLast_Kick_Times;

		std::weak_ptr<CWorker_Pool> mWorker_Pool;

	protected:
		void Watchdog_Thread_Fnc();

		void Write_Status();

	public:
		CWatchdog();
		~CWatchdog();

		CWatchdog(const CWatchdog&) = delete;
		CWatchdog& operator=(const CWatchdog&) = delete;

		void Start(const std::string& wdFilePath, std::weak_ptr<CWorker_Pool> workerPool);
		void Stop();
		void Kick(NWatchdog_Source source);

		static CWatchdog& Get_Instance() {
			return sInstance;
		}
};
