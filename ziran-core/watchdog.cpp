#include "watchdog.h"

#include <iostream>
#include <chrono>
#include <thread>

#include <spdlog/spdlog.h>

CWatchdog CWatchdog::sInstance;

CWatchdog::CWatchdog() {
	//
}

CWatchdog::~CWatchdog() {
	//
}

void CWatchdog::Start(const std::string& wdFilePath) {
	if (mRunning) {
		return;
	}

	mWatchdog_File.open(wdFilePath, std::ios::out | std::ios::trunc);
	if (!mWatchdog_File.is_open()) {
		throw std::runtime_error("Failed to open watchdog file: " + wdFilePath);
	}

	// initialize last kick times
	auto now = std::chrono::steady_clock::now();
	for (size_t i = static_cast<size_t>(NWatchdog_Source::start); i <= static_cast<size_t>(NWatchdog_Source::end); ++i) {
		mLast_Kick_Times[static_cast<NWatchdog_Source>(i)] = now;
	}

	Write_Status();

	mRunning = true;
	mWD_Thread = std::make_unique<std::thread>(&CWatchdog::Watchdog_Thread_Fnc, this);
}

void CWatchdog::Stop() {
	if (!mRunning) {
		return;
	}

	// lock scope
	{
		std::lock_guard<std::mutex> lock(mMtx);
		mRunning = false;
	}

	mCv.notify_all();
	if (mWD_Thread && mWD_Thread->joinable()) {
		mWD_Thread->join();
	}
	mWD_Thread.reset();
}

void CWatchdog::Kick(NWatchdog_Source source) {
	std::lock_guard<std::mutex> lock(mMtx);
	mLast_Kick_Times[source] = std::chrono::steady_clock::now();
}

void CWatchdog::Watchdog_Thread_Fnc() {
	const auto check_interval = std::chrono::milliseconds(1000);
	while (true) {
		// wait for the next check or stop signal
		{
			std::unique_lock<std::mutex> lock(mMtx);
			if (!mRunning) {
				break;
			}

			mCv.wait_for(lock, check_interval);
			if (!mRunning) {
				break;
			}
		}

		// check kick times
		bool expired = false;
		{
			std::lock_guard<std::mutex> lock(mMtx);
			auto now = std::chrono::steady_clock::now();
			for (const auto& [source, kick_time] : mLast_Kick_Times) {
				auto elapsed = now - kick_time;
				if (elapsed > std::chrono::milliseconds(Watchdog_Kick_Timeout_MS)) {
					expired = true;
					break;
				}
			}
		}

		// update status
		{
			std::lock_guard<std::mutex> lock(mMtx);
			if (expired) {
				if (mStatus != NWatchdog_Status::Expired) {
					spdlog::error("Watchdog expired!");
				}
				mStatus = NWatchdog_Status::Expired;
			} else {
				mStatus = NWatchdog_Status::Operational;
			}
		}

		// write status to file
		Write_Status();
	}
}

void CWatchdog::Write_Status() {
	mWatchdog_File.seekp(0);

	// output current timestamp in seconds since epoch
	auto now = std::chrono::system_clock::now();
	auto now_s = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
	mWatchdog_File << "TIME: " << now_s << std::endl;

	mWatchdog_File << "STATUS: ";
	switch (mStatus) {
		case NWatchdog_Status::Inactive:
			mWatchdog_File << "Inactive";
			break;
		case NWatchdog_Status::Operational:
			mWatchdog_File << "Operational";
			break;
		case NWatchdog_Status::Expired:
			mWatchdog_File << "Expired";
			break;
		case NWatchdog_Status::Terminated:
			mWatchdog_File << "Terminated";
			break;
	}
	mWatchdog_File << std::endl;
	if (mStatus == NWatchdog_Status::Expired) {
		mWatchdog_File << "EXPIRED_SOURCES: ";
		for (const auto& [source, kick_time] : mLast_Kick_Times) {

			auto elapsed = std::chrono::steady_clock::now() - kick_time;
			if (elapsed > std::chrono::milliseconds(Watchdog_Kick_Timeout_MS)) {
				switch (source) {
					case NWatchdog_Source::Periodic_Check:
						mWatchdog_File << "Periodic_Check ";
						break;
					case NWatchdog_Source::Job_Manager:
						mWatchdog_File << "Job_Manager ";
						break;
				}
			}
		}
		mWatchdog_File << std::endl;
	}

	mWatchdog_File.flush();
}
