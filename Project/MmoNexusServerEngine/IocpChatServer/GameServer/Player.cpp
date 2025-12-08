#include "pch.h"
#include "Player.h"
#include "PlayerSession.h" // Session 기능을 쓰려면 포함
#include "GameRoom.h"
#include "DataManager.h"

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

void Player::RefreshStats()
{
	// 1. 기본 정보 가져오기
	Protocol::StatInfo* stat = GetStatInfo();

	// [DataManager] 1-1. 레벨에 맞는 기본 스탯(Base Stat) 조회
	const Protocol::StatTemplateInfo* baseStat = DataManager::Instance()->GetStatTemplate(stat->level());
	if (baseStat == nullptr)
	{
		// 데이터가 없으면 비상용 기본값 (혹은 1레벨로 강제)
		// 실제론 로그 찍고 처리해야 함
		return;
	}

	// 1-2. 기본 스탯 적용 (Reset)
	stat->set_maxhp(baseStat->maxhp());
	stat->set_attack(baseStat->attack());
	stat->set_defense(baseStat->defense());
	stat->set_speed(baseStat->speed());
	stat->set_totalexp(baseStat->totalexp()); // 필요하면

	// 2. 장착 아이템 스탯 합산 (Item Bonus)
	for (const auto& item : _items)
	{
		// 장착 중인 아이템만 계산
		if (item.isequipped())
		{
			// [DataManager] 2-1. 아이템 템플릿 정보 조회
			const Protocol::ItemTemplateInfo* itemData = DataManager::Instance()->GetItemTemplate(item.templateid());
			if (itemData)
			{
				// 2-2. 스탯 더하기
				stat->set_maxhp(stat->maxhp() + itemData->hpbonus());
				stat->set_attack(stat->attack() + itemData->attackbonus());
				stat->set_defense(stat->defense() + itemData->defensebonus());
				// Speed 등 다른 옵션이 있다면 여기서 추가
			}
		}
	}

	// 3. 체력 보정 (최대 체력이 줄어들었을 때 현재 체력이 오버되지 않게)
	if (stat->hp() > stat->maxhp())
		stat->set_hp(stat->maxhp());

	// [Log] 디버깅용 (나중에 삭제)
	std::cout << "💪 [Stat Refresh] HP:" << stat->maxhp() << " ATK:" << stat->attack() << " DEF:" << stat->defense() << std::endl;
}

void Player::Update()
{
	// TODO: 쿨타임 계산, 버프 처리 등 프레임 단위 로직
}