#include "pch.h"
#include "Service.h"
#include "Session.h"
#include "Listener.h"

// ==============================
// Service
// ==============================

Service::Service(ServiceType type, NetAddress address, IocpCoreRef core, SessionFactory factory, int32 maxSessionCount)
	: _type(type), _netAddress(address), _iocpCore(core), _sessionFactory(factory), _maxSessionCount(maxSessionCount)
{
}

Service::~Service()
{
}

void Service::CloseService()
{
	// [GIGACHAD FIX] 데드락 방지 코드
	// 락을 잡은 상태에서 Disconnect를 호출하면, 
	// OnDisconnected -> ReleaseSession에서 다시 락을 잡으려다 서버가 멈출 수 있다.
	// 그래서 세션 목록을 복사해두고, 락을 푼 뒤에 끊어야 한다.

	Set<SessionRef> tempSessions;
	{
		WRITE_LOCK;
		tempSessions = _sessions;
		_sessions.clear();
		_sessionCount = 0;
	}

	for (const auto& session : tempSessions)
	{
		session->Disconnect(L"Server Shutdown");
	}
}

void Service::Broadcast(SendBufferRef sendBuffer)
{
	WRITE_LOCK;
	for (const auto& session : _sessions)
	{
		session->Send(sendBuffer);
	}
}

SessionRef Service::CreateSession()
{
	SessionRef session = _sessionFactory();
	session->SetService(shared_from_this());

	if (_iocpCore->Register(session) == false)
		return nullptr;

	return session;
}

void Service::AddSession(SessionRef session)
{
	WRITE_LOCK;
	_sessionCount++;
	_sessions.insert(session);

	// [GIGACHAD FIX] 콜백 호출 (이게 있어야 LoginServer가 세션을 낚아챈다)
	if (_connectCallback)
		_connectCallback(session);
}

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
	for (auto it = _sessions.begin(); it != _sessions.end(); )
	{
		SessionRef session = *it;

		uint64 lastRecv = session->GetLastRecvTime();

		if (lastRecv != 0 && (now - lastRecv > 30000))
		{
			std::cout << "Heartbeat Timeout!" << std::endl;
			session->Disconnect(L"Heartbeat Timeout");
		}

		if (session->IsConnected() == false)
		{
			it = _sessions.erase(it);
			_sessionCount--;
			continue;
		}

		session->Ping();
		++it;
	}
}

// ==============================
// ClientService
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
		{
			std::cout << " [Warning] Initial Connection Failed. Will retry later via Heartbeat." << std::endl;
			//return false;
		}
			
	}

	return true;
}

void ClientService::CheckHeartbeat()
{
	Service::CheckHeartbeat();

	int32 currCount = GetCurrentSessionCount();
	int32 maxCount = GetMaxSessionCount();

	if (currCount < maxCount)
	{
		for (int32 i = 0; i < maxCount - currCount; i++)
		{
			SessionRef session = CreateSession();
			if (session)
			{
				std::cout << "[System] Auto-Reconnecting..." << std::endl;
				session->Connect();
			}
		}
	}
}

// ==============================
// ServerService
// ==============================

ServerService::ServerService(NetAddress address, IocpCoreRef core, SessionFactory factory, int32 maxSessionCount)
	: Service(ServiceType::Server, address, core, factory, maxSessionCount)
{
}

bool ServerService::Start()
{
	if (CanStart() == false)
		return false;

	_listener = MakeShared<Listener>();
	if (_listener == nullptr)
		return false;

	ServerServiceRef service = static_pointer_cast<ServerService>(shared_from_this());
	if (_listener->StartAccept(service) == false)
		return false;

	return true;
}

void ServerService::CloseService()
{
	if (_listener)
	{
		_listener->CloseAccept();
		_listener = nullptr;
	}

	Service::CloseService();
}