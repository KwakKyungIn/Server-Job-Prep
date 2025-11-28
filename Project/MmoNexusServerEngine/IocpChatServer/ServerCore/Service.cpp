#include "pch.h"
#include "Service.h"
#include "Session.h"
#include "Listener.h"

// ==============================
// Service
// - 서버/클라이언트 서비스의 공통 기반
// - 세션 생성/등록/브로드캐스트/해제 관리
// ==============================

Service::Service(ServiceType type, NetAddress address, IocpCoreRef core, SessionFactory factory, int32 maxSessionCount)
	: _type(type), _netAddress(address), _iocpCore(core), _sessionFactory(factory), _maxSessionCount(maxSessionCount)
{
}

Service::~Service()
{
}

// 서비스 종료 (현재는 TODO, 자원 정리 필요)
void Service::CloseService()
{
	// 1. 영업 끝났다고 선포한다.
	// 2. 현재 붙어있는 모든 세션을 끊어버린다.
	WRITE_LOCK;
	for (const auto& session : _sessions)
	{
		session->Disconnect(L"Server Shutdown");
	}
	_sessions.clear();
	_sessionCount = 0;
}



// 모든 세션에 동일한 메시지 브로드캐스트
// - lock으로 세션 컨테이너 보호
void Service::Broadcast(SendBufferRef sendBuffer)
{
	WRITE_LOCK;
	for (const auto& session : _sessions)
	{
		session->Send(sendBuffer);
	}
}

// 세션 생성
// - 팩토리 함수(SessionFactory)로 실제 세션 객체를 생성
// - Service와 IOCP Core 등록까지 수행
SessionRef Service::CreateSession()
{
	SessionRef session = _sessionFactory();
	session->SetService(shared_from_this());

	if (_iocpCore->Register(session) == false)
		return nullptr;

	return session;
}

// 세션 추가 (연결 성공 시 호출)
// - sessionCount 증가 + 컨테이너에 보관
void Service::AddSession(SessionRef session)
{
	WRITE_LOCK;
	_sessionCount++;
	_sessions.insert(session);
}

// 세션 해제 (연결 종료 시 호출)
// - 반드시 컨테이너에 존재해야 함
void Service::ReleaseSession(SessionRef session)
{
	WRITE_LOCK;
	ASSERT_CRASH(_sessions.erase(session) != 0);
	_sessionCount--;
}

void Service::CheckHeartbeat()
{
	uint64 now = ::GetTickCount64();

	WRITE_LOCK;
	// iterator 무효화 방지를 위한 안전한 루프
	for (auto it = _sessions.begin(); it != _sessions.end(); )
	{
		SessionRef session = *it;

		// 1. 타임아웃 체크
		uint64 lastRecv = session->GetLastRecvTime();

		if (lastRecv != 0 && (now - lastRecv > 30000))
		{
			std::cout << "Heartbeat Timeout!" << std::endl;

			// 여기서 Disconnect를 호출하면 -> RegisterDisconnect 실패 -> return false.
			// Session은 여전히 _sessions에 남아있음.
			// 하지만 _connected 상태는 false가 됨(Disconnect 내부 로직).
			session->Disconnect(L"Heartbeat Timeout");
		}

		// 2. 연결 끊긴 세션 정리 (Garbage Collection)
		// DisconnectEx가 실패해서 ReleaseSession이 안 불린 놈들을 여기서 수동으로 지운다.
		if (session->IsConnected() == false)
		{
			// [중요] erase를 하면 it가 무효화되므로, 반환값을 받아야 함.
			it = _sessions.erase(it);
			_sessionCount--; // 카운트 감소

			// 이미 지웠으니 아래 코드는 실행하면 안 됨. Loop 계속.
			continue;
		}

		// 3. 살아있으면 Ping
		session->Ping();

		// 다음 세션으로
		++it;
	}
}


// ==============================
// ClientService
// - 클라이언트 전용 Service
// - 지정한 개수만큼 Connect 시도
// ==============================

ClientService::ClientService(NetAddress targetAddress, IocpCoreRef core, SessionFactory factory, int32 maxSessionCount)
	: Service(ServiceType::Client, targetAddress, core, factory, maxSessionCount)
{
}

bool ClientService::Start()
{
	if (CanStart() == false)
		return false;

	const int32 sessionCount = GetMaxSessionCount();
	for (int32 i = 0; i < sessionCount; i++)
	{
		SessionRef session = CreateSession();
		if (session->Connect() == false)
			return false;
	}

	return true;
}

// [ADD] 재접속 로직 (Auto-Reconnect)
void ClientService::CheckHeartbeat()
{
	// 1. 부모의 좀비 킬링 로직 먼저 수행
	Service::CheckHeartbeat();

	// 2. 부족한 세션 채우기 (재접속)
	// 락을 걸지 않고 sessionCount를 확인해도 되지만(atomic이면), 
	// 정확성을 위해 일단 락 안에서 확인하거나, GetCurrentSessionCount() 사용.

	int32 currCount = GetCurrentSessionCount();
	int32 maxCount = GetMaxSessionCount();

	if (currCount < maxCount)
	{
		// 부족한 만큼 채운다
		for (int32 i = 0; i < maxCount - currCount; i++)
		{
			SessionRef session = CreateSession();
			if (session)
			{
				// Connect는 비동기라 실패할 수도 있지만 일단 던진다.
				// 실패하면 다음 틱에 또 currCount가 부족할 테니 다시 시도할 것이다.
				std::cout << "[System] Auto-Reconnecting..." << std::endl;
				session->Connect();
			}
		}
	}
}

// ==============================
// ServerService
// - 서버 전용 Service
// - Listener를 생성하여 AcceptEx 루프 시작
// ==============================

ServerService::ServerService(NetAddress address, IocpCoreRef core, SessionFactory factory, int32 maxSessionCount)
	: Service(ServiceType::Server, address, core, factory, maxSessionCount)
{
}

bool ServerService::Start()
{
	if (CanStart() == false)
		return false;

	// Listener 생성 및 AcceptEx 준비
	_listener = MakeShared<Listener>();
	if (_listener == nullptr)
		return false;

	// shared_from_this()로 자기 자신을 ServerServiceRef로 변환
	ServerServiceRef service = static_pointer_cast<ServerService>(shared_from_this());
	if (_listener->StartAccept(service) == false)
		return false;

	return true;
}

void ServerService::CloseService()
{
	// 1. 리스너부터 죽인다. (더 이상 신규 유저 안 받음)
	if (_listener)
	{
		_listener->CloseAccept();
		_listener = nullptr;
	}

	// 2. 부모 클래스의 정리(세션 해제) 호출
	Service::CloseService();
}
