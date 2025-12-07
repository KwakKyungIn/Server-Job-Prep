#pragma once
#include <string>
#include <atomic>
// 1. 설정 데이터 구조체 정의
struct ServerConfig
{
	std::wstring DBConnectionString; // DB 연결 문자열
	int32 Port = 0;
	int32 MaxUser = 0;
};

// 2. 전역 설정 변수 선언 (extern)
extern ServerConfig GServerConfig;

// 3. 매니저 전역 객체 포인터 선언 (extern)
// (다른 파일들에서 이 변수들을 가져다 씀)
extern class ThreadManager* GThreadManager;
extern class Memory* GMemory;
extern class SendBufferManager* GSendBufferManager;
extern class DeadLockProfiler* GDeadLockProfiler;
extern class GlobalQueue* GGlobalQueue;
extern std::atomic<bool> GIsRunning;
//extern class DBConnectionPool* GDBConnectionPool;

// 4. CoreGlobal 클래스 선언
// (생성자/소멸자가 cpp에 구현되어 있음)
class CoreGlobal
{
public:
	CoreGlobal();
	~CoreGlobal();
};

extern class CoreGlobal GCoreGlobal;