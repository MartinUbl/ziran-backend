#include "jobmgr.h"
#include "watchdog.h"
#include "defines.h"

#include <array>
#include <string_view>

#include <spdlog/spdlog.h>

#ifdef _WIN32
#include <ws2tcpip.h>
#else
#include <poll.h>
#endif

CJob_Mgr::CJob_Mgr(const TConfig& cfg) : mConfig(cfg) {
	//
}

bool CJob_Mgr::Start() {
	Network_Init();
	mRunning = true;

	mSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if (mSocket < 0) {
		spdlog::error("Could not create socket: {}", Get_Last_Socket_Error());
		return false;
	}

	mMyAddr.sin_family = AF_INET;
	mMyAddr.sin_port = htons(mConfig.bindPort);
	if (inet_pton(AF_INET, mConfig.bindIpString.c_str(), &mMyAddr.sin_addr.s_addr) != 1) {
		spdlog::error("Could not resolve bind host {}", mConfig.bindIpString);
		return false;
	}

	int result;

	result = bind(mSocket, reinterpret_cast<sockaddr*>(&mMyAddr), sizeof(mMyAddr));
	if (result != 0) {
		spdlog::error("Could not bind to address {}:{}. Error: {}", mConfig.bindIpString, mConfig.bindPort, Get_Last_Socket_Error());
		return false;
	}

	mThread = std::make_unique<std::thread>(&CJob_Mgr::Thread_Fnc, this);

	return true;
}

void CJob_Mgr::Stop() {

	// lock scope
	{
		std::unique_lock<std::mutex> lck(mMtx);

		mRunning = false;
		closesocket(mSocket);
	}

	if (mThread && mThread->joinable()) {
		mThread->join();
	}

	Network_Deinit();
}

void CJob_Mgr::Thread_Fnc() {

	sockaddr_in addr;
	SOCKLEN addrLen = sizeof(addr);
	struct pollfd pfd;

	std::array<char, 128> buffer;

	const std::string WakeUpStr{ "WAKEUP" };
	const std::string ReloadStr{ "RELOAD" };

	pfd.fd = mSocket;
	pfd.events = POLLIN;

	while (mRunning) {
		CWatchdog::Get_Instance().Kick(NWatchdog_Source::Job_Manager);

#ifdef WIN32
		int pollResult = WSAPoll(&pfd, 1, 1000);
#else
		int pollResult = poll(&pfd, 1, 1000);
#endif

		// timeout
		if (pollResult <= 0) {
			continue;
		}

		std::fill(buffer.begin(), buffer.end(), '\0');

		int result = recvfrom(mSocket, buffer.data(), static_cast<int>(buffer.size()), 0, reinterpret_cast<sockaddr*>(&addr), &addrLen);
		if (result < buffer.size() - 1) {
			buffer[result] = '\0';
		}

		if (result == static_cast<int>(WakeUpStr.size()) && std::string_view{ buffer.data() } == WakeUpStr) {
			std::unique_lock<std::mutex> lck(mMtx);

			mSignalized = true;
			mCv.notify_all();

			sendto(mSocket, "OK", 2, 0, reinterpret_cast<sockaddr*>(&addr), addrLen);
		}
		else if (result == static_cast<int>(ReloadStr.size()) && std::string_view{ buffer.data() } == ReloadStr) {
			std::unique_lock<std::mutex> lck(mMtx);

			mSignalized = true;
			mShould_Reload = true;
			mCv.notify_all();

			sendto(mSocket, "OK", 2, 0, reinterpret_cast<sockaddr*>(&addr), addrLen);
		}
	}
}

NWakeUp_Reason CJob_Mgr::Await(std::chrono::milliseconds timeout)
{
	std::unique_lock<std::mutex> lck(mMtx);

	while (!mSignalized && mRunning) {
		auto cv_res = mCv.wait_for(lck, timeout);
		if (cv_res == std::cv_status::timeout && !mSignalized) {
			return NWakeUp_Reason::Timeout;
		}
	}

	bool isSuccess = mRunning && mSignalized;

	mSignalized = false;

	if (!isSuccess) {
		return NWakeUp_Reason::None;
	}

	if (mShould_Reload) {
		return NWakeUp_Reason::Reload;
	}

	return NWakeUp_Reason::Job;
}
