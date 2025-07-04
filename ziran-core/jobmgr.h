#pragma once

#include "defines.h"
#include "config.h"

#include <thread>
#include <memory>
#include <mutex>
#include <condition_variable>

enum class NWakeUp_Reason
{
	None,

	Job,
	Reload,
};

class CJob_Mgr
{
	private:
		const TConfig& mConfig;

		std::unique_ptr<std::thread> mThread;
		std::mutex mMtx;
		std::condition_variable mCv;

		bool mRunning = false;
		bool mSignalized = false;
		bool mShould_Reload = false;

	private:
		SOCK mSocket;
		sockaddr_in mMyAddr;

	protected:
		void Thread_Fnc();

	public:
		CJob_Mgr(const TConfig& cfg);

		bool Start(std::vector<std::string>& log);
		void Stop();

		NWakeUp_Reason Await();
};
