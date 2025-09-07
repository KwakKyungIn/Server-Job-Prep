#include "pch.h"
#include "Session.h"
#include "SocketUtils.h"
#include "Service.h"

//테스트용
#include "Metrics.h"
/*--------------
	Session
---------------*/


Session::Session() : _recvBuffer(BUFFER_SIZE)
{
	_socket = SocketUtils::CreateSocket();
}

Session::~Session()
{
	SocketUtils::Close(_socket);
}

void Session::Send(SendBufferRef sendBuffer)
{
	if (IsConnected() == false)
		return;

	bool registerSend = false;
	const int32 sz = static_cast<int32>(sendBuffer->WriteSize());

	{
		WRITE_LOCK;

		// --- 하드캡: 너무 느린 세션 보호 (브로드캐스트라도 과감히 드롭) ---
		if ((_sendBacklogBytes + sz > MAX_BACKLOG_BYTES) ||
			(_sendBacklogCount + 1 > MAX_BACKLOG_COUNT))
		{
			// 필요시 강종 정책으로 바꾸려면 아래 주석 해제
			// Disconnect("send backlog");
			return; // 드롭
		}

		// --- 소프트캡: 이미 WSASend 진행중이면 더 빡세게 드롭 ---
		if (_sendRegistered.load(std::memory_order_relaxed))
		{
			if ((_sendBacklogBytes >= SOFT_BACKLOG_BYTES_WHEN_REGISTERED) ||
				(_sendBacklogCount >= SOFT_BACKLOG_COUNT_WHEN_REGISTERED))
			{
				return; // 드롭
			}
		}

		_sendQueue.push(sendBuffer);
		_sendBacklogBytes += sz;
		_sendBacklogCount += 1;

		if (_sendRegistered.exchange(true) == false)
			registerSend = true;
	}

	if (registerSend)
		RegisterSend();
}

bool Session::Connect()
{
	return RegisterConnect();
}

void Session::Disconnect(const WCHAR* cause)
{
	if (_connected.exchange(false) == false)
		return;

	// TEMP
	wcout << "Disconnect : " << cause << endl;

	RegisterDisconnect();
}

HANDLE Session::GetHandle()
{
	return reinterpret_cast<HANDLE>(_socket);
}

void Session::Dispatch(IocpEvent* iocpEvent, int32 numOfBytes)
{
	switch (iocpEvent->eventType)
	{
	case EventType::Connect:
		ProcessConnect();
		break;
	case EventType::Disconnect:
		ProcessDisconnect();
		break;
	case EventType::Recv:
		ProcessRecv(numOfBytes);
		break;
	case EventType::Send:
		ProcessSend(numOfBytes);
		break;
	default:
		break;
	}
}

bool Session::RegisterConnect()
{
	if (IsConnected())
		return false;

	if (GetService()->GetServiceType() != ServiceType::Client)
		return false;

	if (SocketUtils::SetReuseAddress(_socket, true) == false)
		return false;

	if (SocketUtils::BindAnyAddress(_socket, 0/*남는거*/) == false)
		return false;

	_connectEvent.Init();
	_connectEvent.owner = shared_from_this(); // ADD_REF

	DWORD numOfBytes = 0;
	SOCKADDR_IN sockAddr = GetService()->GetNetAddress().GetSockAddr();
	if (false == SocketUtils::ConnectEx(_socket, reinterpret_cast<SOCKADDR*>(&sockAddr), sizeof(sockAddr), nullptr, 0, &numOfBytes, &_connectEvent))
	{
		int32 errorCode = ::WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			_connectEvent.owner = nullptr; // RELEASE_REF
			return false;
		}
	}

	return true;
}

bool Session::RegisterDisconnect()
{
	_disconnectEvent.Init();
	_disconnectEvent.owner = shared_from_this(); // ADD_REF

	if (false == SocketUtils::DisconnectEx(_socket, &_disconnectEvent, TF_REUSE_SOCKET, 0))
	{
		int32 errorCode = ::WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			_disconnectEvent.owner = nullptr; // RELEASE_REF
			return false;
		}
	}

	return true;
}

void Session::RegisterRecv()
{
	if (IsConnected() == false)
		return;

	_recvEvent.Init();
	_recvEvent.owner = shared_from_this(); // ADD_REF

	WSABUF wsaBuf;
	wsaBuf.buf = reinterpret_cast<char*>(_recvBuffer.WritePos());
	wsaBuf.len = _recvBuffer.FreeSize();

	DWORD numOfBytes = 0;
	DWORD flags = 0;
	if (SOCKET_ERROR == ::WSARecv(_socket, &wsaBuf, 1, OUT & numOfBytes, OUT & flags, &_recvEvent, nullptr))
	{
		int32 errorCode = ::WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			HandleError(errorCode);
			_recvEvent.owner = nullptr; // RELEASE_REF
		}
	}
}

// Session::RegisterSend() 수정 핵심
void Session::RegisterSend()
{
	if (IsConnected() == false)
		return;

	_sendEvent.Init();
	_sendEvent.owner = shared_from_this(); // ADD_REF

	// --- 배치 상한 적용: 한 번에 너무 많이 묶지 않도록 ---
	{
		WRITE_LOCK;

		int32 batchedBytes = 0;
		while (_sendQueue.empty() == false)
		{
			SendBufferRef sb = _sendQueue.front();
			const int32 sz = static_cast<int32>(sb->WriteSize());

			// 첫 요소는 예외적으로 허용, 그 뒤부터 상한 체크
			if (!_sendEvent.sendBuffers.empty())
			{
				if (batchedBytes + sz > MAX_SEND_BATCH_BYTES) break;
				if ((int32)_sendEvent.sendBuffers.size() >= MAX_SEND_BATCH_COUNT) break;
			}

			_sendQueue.pop();
			_sendEvent.sendBuffers.push_back(sb);
			batchedBytes += sz;

			// 큐 -> 이벤트 이동했으니 백로그 장부 차감
			_sendBacklogBytes -= sz;
			_sendBacklogCount -= 1;
		}
	}

	// Scatter-Gather WSASend
	Vector<WSABUF> wsaBufs;
	wsaBufs.reserve(_sendEvent.sendBuffers.size());
	for (SendBufferRef sb : _sendEvent.sendBuffers)
	{
		WSABUF b;
		b.buf = reinterpret_cast<char*>(sb->Buffer());
		b.len = static_cast<ULONG>(sb->WriteSize());
		wsaBufs.push_back(b);
	}

	DWORD numOfBytes = 0;
	if (SOCKET_ERROR == ::WSASend(_socket, wsaBufs.data(), (DWORD)wsaBufs.size(), OUT & numOfBytes, 0, &_sendEvent, nullptr))
	{
		const int32 err = ::WSAGetLastError();
		if (err != WSA_IO_PENDING)
		{
			HandleError(err);
			_sendEvent.owner = nullptr;      // RELEASE_REF
			_sendEvent.sendBuffers.clear();  // RELEASE_REF
			_sendRegistered.store(false);
		}
	}
}
void Session::ProcessConnect()
{
	_connectEvent.owner = nullptr; // RELEASE_REF

	_connected.store(true);

	// 세션 등록
	GetService()->AddSession(GetSessionRef());

	// 컨텐츠 코드에서 재정의
	OnConnected();

	// 수신 등록
	RegisterRecv();
}

void Session::ProcessDisconnect()
{
	_disconnectEvent.owner = nullptr; // RELEASE_REF

	OnDisconnected(); // 컨텐츠 코드에서 재정의
	GetService()->ReleaseSession(GetSessionRef());
}

// === 변경 1: ProcessRecv() 내 계측 수정 ===
void Session::ProcessRecv(int32 numOfBytes)
{
	_recvEvent.owner = nullptr; // RELEASE_REF

	if (numOfBytes == 0)
	{
		Disconnect(L"Recv 0");
		return;
	}

	if (_recvBuffer.OnWrite(numOfBytes) == false)
	{
		Disconnect(L"OnWrite Overflow");
		return;
	}

	// [METRICS] IOCP Recv 완료 개수/바이트 (명시적으로 io_* 사용)
	GMetrics.io_recv_ops.fetch_add(1, std::memory_order_relaxed);
	GMetrics.bytes_recv.fetch_add((uint64_t)numOfBytes, std::memory_order_relaxed);

	int32 dataSize = _recvBuffer.DataSize();
	int32 processLen = OnRecv(_recvBuffer.ReadPos(), dataSize); // 컨텐츠 코드에서 재정의
	if (processLen < 0 || dataSize < processLen || _recvBuffer.OnRead(processLen) == false)
	{
		Disconnect(L"OnRead Overflow");
		return;
	}

	// 커서 정리
	_recvBuffer.Clean();

	// 수신 등록
	RegisterRecv();
}

// === 변경 2: ProcessSend() 내 계측 수정 ===
void Session::ProcessSend(int32 numOfBytes)
{
	_sendEvent.owner = nullptr; // RELEASE_REF
	_sendEvent.sendBuffers.clear(); // RELEASE_REF

	if (numOfBytes == 0)
	{
		Disconnect(L"Send 0");
		return;
	}

	// [METRICS] IOCP Send 완료 개수/바이트 (명시적으로 io_* 사용)
	GMetrics.io_send_ops.fetch_add(1, std::memory_order_relaxed);
	GMetrics.bytes_sent.fetch_add((uint64_t)numOfBytes, std::memory_order_relaxed);

	// 컨텐츠 코드에서 재정의
	OnSend(numOfBytes);

	WRITE_LOCK;
	if (_sendQueue.empty())
		_sendRegistered.store(false);
	else
		RegisterSend();
}

void Session::HandleError(int32 errorCode)
{
	switch (errorCode)
	{
	case WSAECONNRESET:
	case WSAECONNABORTED:
		Disconnect(L"HandleError");
		break;
	default:
		// TODO : Log
		cout << "Handle Error : " << errorCode << endl;
		break;
	}
}

/*-----------------
	PacketSession
------------------*/

PacketSession::PacketSession()
{
}

PacketSession::~PacketSession()
{
}

// [size(2)][id(2)][data....][size(2)][id(2)][data....]
int32 PacketSession::OnRecv(BYTE* buffer, int32 len)
{
	int32 processLen = 0;

	while (true)
	{
		int32 dataSize = len - processLen;
		// 최소한 헤더는 파싱할 수 있어야 한다
		if (dataSize < sizeof(PacketHeader))
			break;

		PacketHeader header = *(reinterpret_cast<PacketHeader*>(&buffer[processLen]));
		// 헤더에 기록된 패킷 크기를 파싱할 수 있어야 한다
		if (dataSize < header.size)
			break;

		// 패킷 조립 성공
		OnRecvPacket(&buffer[processLen], header.size);

		processLen += header.size;
	}

	return processLen;
}
