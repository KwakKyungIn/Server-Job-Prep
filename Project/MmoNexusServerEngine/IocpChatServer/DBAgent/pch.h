#pragma once

#define WIN32_LEAN_AND_MEAN // 거의 사용되지 않는 내용을 Windows 헤더에서 제외합니다.

#ifdef _DEBUG
#pragma comment(lib, "ServerCore\\Debug\\ServerCore.lib")
#pragma comment(lib, "Protobuf\\Debug\\libprotobufd.lib")
#else
#pragma comment(lib, "ServerCore\\Release\\ServerCore.lib")
#pragma comment(lib, "Protobuf\\Release\\libprotobuf.lib")
#endif


#include "CorePch.h"

// 2. ODBC 헤더 (필수)
#include <sql.h>
#include <sqlext.h>

// 3. DBConnectionPool 헤더
#include "DBConnectionPool.h"

// 4. [선언] "GDBConnectionPool이라는 전역 포인터가 어딘가에 있을 거야"라고 공표
extern DBConnectionPool* GDBConnectionPool;