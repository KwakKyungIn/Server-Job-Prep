#pragma once
#include "Session.h"

// DB 에이전트와의 연결을 담당하는 세션 클래스
// PacketSession 상속받아서 패킷 송수신 처리함
class DBSession : public PacketSession
{
public:
	DBSession() {}
	virtual ~DBSession() {}

	// 연결되거나 끊길 때, 패킷 받았을 때 호출되는 콜백 함수들
	virtual void OnConnected() override;
	virtual void OnDisconnected() override;
	virtual void OnRecvPacket(BYTE* buffer, int32 len) override;
	virtual void OnSend(int32 len) override;

	// 주기적인 헬스 체크용
	virtual void Ping() override;
};