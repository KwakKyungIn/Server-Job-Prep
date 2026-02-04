#include "pch.h"
#include "ThreadManager.h"
#include "LoadClientConfig.h"
#include "LoadClientManager.h"
#include "ServerPacketHandler.h"

#include <atomic>
#include <chrono>
#include <iostream>
#include <string>
#include <algorithm>

std::atomic<bool> g_isRunning = true;
PacketSessionRef g_session = nullptr; // legacy symbol for ServerSession.cpp

BOOL WINAPI CtrlHandler(DWORD fdwCtrlType)
{
	switch (fdwCtrlType)
	{
	case CTRL_C_EVENT:
	case CTRL_CLOSE_EVENT:
	case CTRL_LOGOFF_EVENT:
	case CTRL_SHUTDOWN_EVENT:
		std::cout << "\n [LoadClient] Shutdown Initiated..." << std::endl;
		g_isRunning = false;
		return TRUE;
	default:
		return FALSE;
	}
}

int main(int argc, char** argv)
{
	SetConsoleCtrlHandler(CtrlHandler, TRUE);
	ServerPacketHandler::Init();

	std::string configPath = "LoadClientConfig.json";
	if (argc > 1)
		configPath = argv[1];

	LoadClientConfig config;
	std::string error;
	if (!LoadClientConfig::LoadFromFile(configPath, config, error))
	{
		std::cout << "[LoadClient] Config load failed: " << error << std::endl;
		return 1;
	}

	LoadClientManager manager;
	if (!manager.Init(config))
	{
		std::cout << "[LoadClient] Manager init failed." << std::endl;
		return 1;
	}

	manager.Start();

	const int ioThreads = (std::max)(1, config.options.ioThreads);
	IocpCoreRef core = manager.GetIocpCore();

	for (int i = 0; i < ioThreads; ++i)
	{
		GThreadManager->Launch([core]()
			{
				while (g_isRunning)
				{
					core->Dispatch(10);
				}
			});
	}

	while (g_isRunning && manager.IsRunning())
	{
		const uint64 now = ::GetTickCount64();
		manager.Update(now);
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	manager.Stop();
	g_isRunning = false;

	GThreadManager->Join();
	std::cout << "[LoadClient] Finished." << std::endl;
	return 0;
}
