#pragma once
#include "IocpCore.h"
#include "IocpEvent.h"
#include "NetAddress.h"
#include "RecvBuffer.h"

class Service;

class Session : public IocpObject
{
	friend class Listener;
	friend class IocpCore;
	friend class Service;

	enum
	{
		BUFFER_SIZE = 0x10000, // 64KB
	};

public:
	Session();
	virtual ~Session();

	// -------- 외부 API --------
	void Send(SendBufferRef sendBuffer);
	bool Connect();
	void Disconnect(const WCHAR* cause);

	shared_ptr<Service> GetService() { return _service.lock(); }
	void SetService(shared_ptr<Service> service) { _service = service; }

	// -------- 연결 정보 --------
	void SetNetAddress(NetAddress address) { _netAddress = address; }
	NetAddress GetAddress() { return _netAddress; }
	SOCKET GetSocket() { return _socket; }
	bool IsConnected() { return _connected; }
	SessionRef GetSessionRef() { return static_pointer_cast<Session>(shared_from_this()); }

	// [Gigachad] Base Level Session ID
	// 모든 세션(Client, DB, Chat)은 태어날 때 고유 ID를 부여받는다.
	uint64 GetSessionId() { return _sessionId; }

	// -------- Heartbeat --------
	uint64 GetLastSendTime() { return _lastSendTime; }
	uint64 GetLastRecvTime() { return _lastRecvTime; }
	void UpdateLastSendTime() { _lastSendTime = ::GetTickCount64(); }

	// -------- 송신 정책 상수 --------
	static constexpr int32 MAX_SEND_BATCH_BYTES = 32 * 1024;
	static constexpr int32 MAX_SEND_BATCH_COUNT = 64;
	static constexpr int64 MAX_BACKLOG_BYTES = 128 * 1024;
	static constexpr int32 MAX_BACKLOG_COUNT = 256;
	static constexpr int64 SOFT_BACKLOG_BYTES_WHEN_REGISTERED = 64 * 1024;
	static constexpr int32 SOFT_BACKLOG_COUNT_WHEN_REGISTERED = 128;

private:
	virtual HANDLE GetHandle() override;
	virtual void Dispatch(class IocpEvent* iocpEvent, int32 numOfBytes = 0) override;

	bool RegisterConnect();
	bool RegisterDisconnect();
	void RegisterRecv();
	void RegisterSend();

	void ProcessConnect();
	void ProcessDisconnect();
	void ProcessRecv(int32 numOfBytes);
	void ProcessSend(int32 numOfBytes);

	void HandleError(int32 errorCode);

protected:
	virtual void OnConnected() {}
	virtual int32 OnRecv(BYTE* buffer, int32 len) { return len; }
	virtual void OnSend(int32 len) {}
	virtual void OnDisconnected() {}
	virtual void Ping() {}

private:
	weak_ptr<Service> _service;
	SOCKET _socket = INVALID_SOCKET;
	NetAddress _netAddress = {};
	Atomic<bool> _connected = false;

	// [Gigachad] Moved from PlayerSession
	uint64 _sessionId = 0;

	Atomic<uint64> _lastSendTime = 0;
	Atomic<uint64> _lastRecvTime = 0;

private:
	USE_LOCK;
	RecvBuffer _recvBuffer;
	Queue<SendBufferRef> _sendQueue;
	Atomic<bool> _sendRegistered = false;
	int64 _sendBacklogBytes = 0;
	int32 _sendBacklogCount = 0;

	ConnectEvent _connectEvent;
	DisconnectEvent _disconnectEvent;
	RecvEvent _recvEvent;
	SendEvent _sendEvent;
};

struct PacketHeader
{
	uint16 size;
	uint16 id;
};

class PacketSession : public Session
{
public:
	PacketSession();
	virtual ~PacketSession();
	PacketSessionRef GetPacketSessionRef() { return static_pointer_cast<PacketSession>(shared_from_this()); }

protected:
	virtual int32 OnRecv(BYTE* buffer, int32 len) sealed;
	virtual void OnRecvPacket(BYTE* buffer, int32 len) abstract;
};