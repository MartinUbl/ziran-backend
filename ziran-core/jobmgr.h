#pragma once

#include "defines.h"
#include "config.h"

#include <thread>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <vector>

enum class NWakeUp_Reason {
	None,

	Job,
	Reload,
	Timeout,
};

class CJob_Mgr {
	private:
		const TConfig& mConfig;

		std::unique_ptr<std::thread> mThread;
		std::mutex mMtx;
		std::condition_variable mCv;

		bool mRunning = false;
		bool mSignalized = false;
		bool mShould_Reload = false;

	private:
		SOCK mSocket = Invalid_Socket;
		sockaddr_in mMyAddr = {};

	protected:
		void Thread_Fnc();

	public:
		CJob_Mgr(const TConfig& cfg);

		CJob_Mgr(const CJob_Mgr&) = delete;
		CJob_Mgr& operator=(const CJob_Mgr&) = delete;

		bool Start();
		void Stop();

		NWakeUp_Reason Await(std::chrono::milliseconds timeout);
};
