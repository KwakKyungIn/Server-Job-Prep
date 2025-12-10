#pragma once
#include "NetAddress.h"
#include "IocpCore.h"
#include "Listener.h"
#include <functional>

enum class ServiceType : uint8
{
	Server,
	Client
};

using SessionFactory = function<SessionRef(void)>;

// [NEW] 연결 성공 시 호출될 콜백 함수 타입 정의
using ConnectCallback = function<void(SessionRef)>;

class Service : public enable_shared_from_this<Service>
{
public:
	Service(ServiceType type, NetAddress address, IocpCoreRef core, SessionFactory factory, int32 maxSessionCount = 1);
	virtual ~Service();

	virtual bool Start() abstract;
	bool CanStart() { return _sessionFactory != nullptr; }

	virtual void CloseService();
	void SetSessionFactory(SessionFactory func) { _sessionFactory = func; }

	// [NEW] 연결 콜백 설정 함수
	void SetConnectCallback(ConnectCallback func) { _connectCallback = func; }

	void Broadcast(SendBufferRef sendBuffer);
	SessionRef CreateSession();
	void AddSession(SessionRef session);
	void ReleaseSession(SessionRef session);

	int32 GetCurrentSessionCount() { return _sessionCount; }
	int32 GetMaxSessionCount() { return _maxSessionCount; }

	virtual void CheckHeartbeat();

public:
	ServiceType GetServiceType() { return _type; }
	NetAddress GetNetAddress() { return _netAddress; }
	IocpCoreRef& GetIocpCore() { return _iocpCore; }

protected:
	USE_LOCK;
	ServiceType _type;
	NetAddress _netAddress;
	IocpCoreRef _iocpCore;

	Set<SessionRef> _sessions;
	int32 _sessionCount = 0;
	int32 _maxSessionCount = 0;
	SessionFactory _sessionFactory;

	// [NEW] 연결 콜백 저장 변수
	ConnectCallback _connectCallback;
};

// ==============================
// ClientService
// ==============================
class ClientService : public Service
{
public:
	ClientService(NetAddress targetAddress, IocpCoreRef core, SessionFactory factory, int32 maxSessionCount = 1);
	virtual ~ClientService() {}

	virtual bool Start() override;
	virtual void CheckHeartbeat() override; // 재접속 로직
};

// ==============================
// ServerService
// ==============================
class ServerService : public Service
{
public:
	ServerService(NetAddress targetAddress, IocpCoreRef core, SessionFactory factory, int32 maxSessionCount = 1);
	virtual ~ServerService() {}

	virtual bool Start() override;
	virtual void CloseService() override;

private:
	ListenerRef _listener = nullptr;
};