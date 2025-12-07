#include "pch.h"
#include "CoreGlobal.h"
#include "ThreadManager.h"
#include "Memory.h"
#include "DeadLockProfiler.h"
#include "SocketUtils.h"
#include "SendBuffer.h"
#include "GlobalQueue.h"

//  JSON 라이브러리 필수
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

// -----------------------------------------------------------
// 1. 전역 변수 '실체' 정의 (Header에 있는 extern들의 메모리 할당)
// -----------------------------------------------------------
ThreadManager* GThreadManager = nullptr;
Memory* GMemory = nullptr;
SendBufferManager* GSendBufferManager = nullptr;
DeadLockProfiler* GDeadLockProfiler = nullptr;
GlobalQueue* GGlobalQueue = nullptr;
std::atomic<bool> GIsRunning = true;
//DBConnectionPool* GDBConnectionPool = nullptr;

ServerConfig GServerConfig; // 설정값 담을 전역 변수

// -----------------------------------------------------------
// 2. CoreGlobal 구현부
// -----------------------------------------------------------

CoreGlobal::CoreGlobal()
{
	// [Step 1] Config Load (매니저 초기화보다 먼저 실행되어야 함)
	std::ifstream file("ServerConfig.json");
	if (file.is_open())
	{
		json data;
		file >> data;

		// std::string(UTF-8) -> std::wstring(Unicode) 변환
		std::string connStr = data["DB"]["ConnectionString"];
		GServerConfig.DBConnectionString.assign(connStr.begin(), connStr.end());

		GServerConfig.Port = data["Server"]["Port"];
		GServerConfig.MaxUser = data["Server"]["MaxUser"];

		std::cout << "[Config] Loaded Successfully." << std::endl;
	}
	else
	{
		// 파일이 없으면 치명적 에러
		std::cout << "[Config] CRITICAL ERROR: ServerConfig.json not found!" << std::endl;
		// ASSERT_CRASH(false); // 필요하면 주석 해제
	}

	// [Step 2] 핵심 매니저 객체들 초기화
	GThreadManager = new ThreadManager();
	GMemory = new Memory();
	GSendBufferManager = new SendBufferManager();
	GDeadLockProfiler = new DeadLockProfiler();
	GGlobalQueue = new GlobalQueue();
	//GDBConnectionPool = new DBConnectionPool();

	// [Step 3] 소켓 라이브러리 초기화
	SocketUtils::Init();

	std::cout << "CoreGlobal Initialized" << std::endl;
}

CoreGlobal::~CoreGlobal()
{
	// 객체 메모리 해제
	delete GThreadManager;
	delete GMemory;
	delete GSendBufferManager;
	delete GDeadLockProfiler;
	delete GGlobalQueue;
	//delete GDBConnectionPool;

	// 소켓 라이브러리 정리
	SocketUtils::Clear();
}

// -----------------------------------------------------------
// 3. 전역 실행 객체 (이게 있어야 프로그램 시작 시 생성자 호출됨)
// -----------------------------------------------------------
CoreGlobal GCoreGlobal;