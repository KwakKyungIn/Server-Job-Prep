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
	uint64 GetSessionId() { return _sessionId; }

	// -------- Security (Moved Up from PacketSession) --------
	// 부모 클래스인 Session에서 관리해야 Send() 함수에서 접근 가능함
	uint32 GenerateSendSeq() { return _sendSeq.fetch_add(1); }

	bool CheckRecvSeq(uint32 seq)
	{
		if (seq <= _recvSeq) return false;
		_recvSeq = seq;
		return true;
	}

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

	// [Gigachad] Security Variables
	std::atomic<uint32> _sendSeq = 1;
	uint32 _recvSeq = 0;

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
	uint32 crc;
	uint32 seq;
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