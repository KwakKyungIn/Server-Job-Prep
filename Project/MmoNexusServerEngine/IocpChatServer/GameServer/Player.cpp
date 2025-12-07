#include "pch.h"
#include "Player.h"
#include "PlayerSession.h" // Session 기능을 쓰려면 포함
#include "GameRoom.h"

Player::Player()
{
	// [Safety] 기본 초기화 (Init 호출 전 접근 방지용)
	_playerInfo.set_playerid(0);
	_playerInfo.set_name("Uninitialized");
	_playerInfo.set_type(Protocol::PLAYER_NONE);

	Protocol::PositionInfo* pos = _playerInfo.mutable_posinfo();
	pos->set_state(Protocol::MOVE_IDLE);
	pos->set_x(0.0f);
	pos->set_y(0.0f);
	pos->set_z(0.0f);
	pos->set_yaw(0.0f);
}

Player::~Player()
{
	// 디버깅용 소멸자 로그
	// printf("~Player Destructed ID: %llu\n", GetPlayerId());
}

void Player::Init(const Protocol::PlayerInfo& info)
{
	// [Deep Copy] 세션(DB) 데이터를 내 로컬 데이터로 복사
	_playerInfo.CopyFrom(info);

	// [Data Correction] 필수 데이터가 비어있을 경우를 대비한 방어 코드
	if (_playerInfo.has_posinfo() == false)
	{
		Protocol::PositionInfo* pos = _playerInfo.mutable_posinfo();
		pos->set_state(Protocol::MOVE_IDLE);
		pos->set_x(0.0f);
		pos->set_y(0.0f);
		pos->set_z(0.0f);
		pos->set_yaw(0.0f);
	}
}

void Player::Update()
{
	// TODO: 쿨타임 계산, 버프 처리 등 프레임 단위 로직
}