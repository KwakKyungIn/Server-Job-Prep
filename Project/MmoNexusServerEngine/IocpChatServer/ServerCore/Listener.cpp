#include "pch.h"
#include "Listener.h"
#include "SocketUtils.h"
#include "IocpEvent.h"
#include "Session.h"
#include "Service.h"

Listener::~Listener()
{
    // 소켓 닫기
    SocketUtils::Close(_socket);

    // 이벤트 해제
    for (AcceptEvent* acceptEvent : _acceptEvents)
    {
        xdelete(acceptEvent);
    }
}

bool Listener::StartAccept(ServerServiceRef service)
{
    _service = service;
    if (_service == nullptr) return false;

    _socket = SocketUtils::CreateSocket();
    if (_socket == INVALID_SOCKET) return false;

    if (_service->GetIocpCore()->Register(shared_from_this()) == false) return false;

    if (SocketUtils::SetReuseAddress(_socket, true) == false) return false;
    if (SocketUtils::SetLinger(_socket, 0, 0) == false) return false;
    if (SocketUtils::Bind(_socket, _service->GetNetAddress()) == false) return false;
    if (SocketUtils::Listen(_socket) == false) return false;

    const int32 acceptCount = _service->GetMaxSessionCount();
    for (int32 i = 0; i < acceptCount; i++)
    {
        AcceptEvent* acceptEvent = xnew<AcceptEvent>();
        acceptEvent->owner = shared_from_this();
        _acceptEvents.push_back(acceptEvent);

        RegisterAccept(acceptEvent);
    }

    return true;
}

// [수정] 종료 함수
void Listener::CloseAccept()
{
    // 1. 소켓을 닫는다.
    SocketUtils::Close(_socket);

    // 2. [중요] 소켓 변수를 INVALID로 밀어버려야 RegisterAccept가 눈치챈다.
    _socket = INVALID_SOCKET;
}

HANDLE Listener::GetHandle()
{
    return reinterpret_cast<HANDLE>(_socket);
}

void Listener::Dispatch(IocpEvent* iocpEvent, int32 numOfBytes)
{
    ASSERT_CRASH(iocpEvent->eventType == EventType::Accept);
    AcceptEvent* acceptEvent = static_cast<AcceptEvent*>(iocpEvent);

    ProcessAccept(acceptEvent);
}

void Listener::RegisterAccept(AcceptEvent* acceptEvent)
{
    // [CRITICAL FIX] 소켓이 이미 닫혔으면(종료 중이면) 더 이상 등록하지 않는다.
    // 이거 없으면 CloseAccept() 호출 후에 무한 재귀 호출로 스택 오버플로우 남.
    if (_socket == INVALID_SOCKET)
        return;

    SessionRef session = _service->CreateSession();

    acceptEvent->Init();
    acceptEvent->session = session;

    DWORD bytesReceived = 0;
    if (false == SocketUtils::AcceptEx(
        _socket,
        session->GetSocket(),
        session->_recvBuffer.WritePos(),
        0,
        sizeof(SOCKADDR_IN) + 16,
        sizeof(SOCKADDR_IN) + 16,
        OUT & bytesReceived,
        static_cast<LPOVERLAPPED>(acceptEvent)))
    {
        const int32 errorCode = ::WSAGetLastError();
        if (errorCode != WSA_IO_PENDING)
        {
            // [중요] 여기서도 소켓이 유효한지 확인하고 재등록해야 함
            RegisterAccept(acceptEvent);
        }
    }
}

void Listener::ProcessAccept(AcceptEvent* acceptEvent)
{
    SessionRef session = acceptEvent->session;

    // 연결 성공 시 옵션 상속
    if (false == SocketUtils::SetUpdateAcceptSocket(session->GetSocket(), _socket))
    {
        RegisterAccept(acceptEvent);
        return;
    }

    SOCKADDR_IN sockAddress;
    int32 sizeOfSockAddr = sizeof(sockAddress);
    if (SOCKET_ERROR == ::getpeername(session->GetSocket(), OUT reinterpret_cast<SOCKADDR*>(&sockAddress), &sizeOfSockAddr))
    {
        RegisterAccept(acceptEvent);
        return;
    }

    session->SetNetAddress(NetAddress(sockAddress));
    session->ProcessConnect();

    // 다음 연결 받기 위해 재등록
    RegisterAccept(acceptEvent);
}