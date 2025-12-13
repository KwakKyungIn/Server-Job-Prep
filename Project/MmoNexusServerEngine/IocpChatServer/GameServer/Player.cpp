#include "pch.h"
#include "Player.h"
#include "PlayerSession.h" // Session 기능을 쓰려면 포함
#include "GameRoom.h"
#include "DataManager.h"

// [수정 포인트 1] 부모 생성자(Creature) 호출 및 타입 지정
Player::Player() : Creature(Protocol::OBJECT_TYPE_PLAYER)
{
	// [Safety] 기본 초기화
	// PlayerInfo는 Player가 들고 있고, 그 내부 포인터를 Creature(부모)에게 빌려줌
	_playerInfo.set_playerid(0);
	_playerInfo.set_name("Uninitialized");
	_playerInfo.set_type(Protocol::PLAYER_NONE);

	// [수정 포인트 2] 부모(Creature)의 포인터와 내 데이터(PlayerInfo) 연결
	// 이걸 안 하면 Creature 쪽에서 GetPosInfo() 호출할 때 터진다.
	_posInfo = _playerInfo.mutable_posinfo();
	_statInfo = _playerInfo.mutable_statinfo();

	// 초기값 세팅
	_posInfo->set_state(Protocol::MOVE_IDLE);
	_posInfo->set_x(50.0f);
	_posInfo->set_y(0.0f);
	_posInfo->set_z(50.0f);
	_posInfo->set_yaw(0.0f);
}

Player::~Player()
{
	// 디버깅용 소멸자 로그
	// printf("~Player Destructed ID: %llu\n", GetPlayerId());
}

// [수정 포인트 3] 새로 추가된 가상 함수 구현
void Player::OnDamaged(std::shared_ptr<Creature> attacker, int32 damage)
{
	// 1. 부모 로직 (HP 차감 및 사망 체크)
	Creature::OnDamaged(attacker, damage);

	// 2. 플레이어 전용 로직 (예: 피격 사운드, 화면 붉어짐 패킷 등)
	// 현재는 로그만
	// printf("[Player Hit] HP: %d\n", _statInfo->hp());
}

void Player::OnDead(std::shared_ptr<Creature> attacker)
{
	// 1. 부모 로직
	Creature::OnDead(attacker);

	// 2. 플레이어 전용 로직 (예: 부활 UI 팝업, 경험치 하락)
	printf("💀 [Player Dead] %s has been slain!\n", _playerInfo.name().c_str());

	// TODO: 변신 해제, 버프 초기화 등
}

void Player::Init(const Protocol::PlayerInfo& info)
{
	// [Deep Copy] 세션(DB) 데이터를 내 로컬 데이터로 복사
	_playerInfo.CopyFrom(info);

	// [Re-Link] CopyFrom으로 인해 내부 주소가 바뀔 수 있으므로 다시 연결
	_posInfo = _playerInfo.mutable_posinfo();
	_statInfo = _playerInfo.mutable_statinfo();

	// [Data Correction] 필수 데이터가 비어있을 경우를 대비한 방어 코드
	if (_playerInfo.has_posinfo() == false)
	{
		_posInfo->set_state(Protocol::MOVE_IDLE);
		_posInfo->set_x(50.0f);
		_posInfo->set_y(0.0f);
		_posInfo->set_z(50.0f);
		_posInfo->set_yaw(0.0f);
	}
}

void Player::RefreshStats()
{
	// 1. 기본 정보 가져오기 (Creature의 _statInfo 사용)
	Protocol::StatInfo* stat = GetStatInfo();
	if (stat == nullptr) return;

	// [DataManager] 1-1. 레벨에 맞는 기본 스탯(Base Stat) 조회
	const Protocol::StatTemplateInfo* baseStat = DataManager::Instance()->GetStatTemplate(stat->level());
	if (baseStat == nullptr)
	{
		return;
	}

	// 1-2. 기본 스탯 적용 (Reset)
	stat->set_maxhp(baseStat->maxhp());
	stat->set_attack(baseStat->attack());
	stat->set_defense(baseStat->defense());
	stat->set_speed(baseStat->speed());
	stat->set_totalexp(baseStat->totalexp());

	// 2. 장착 아이템 스탯 합산 (Item Bonus)
	for (const auto& item : _items)
	{
		if (item.isequipped())
		{
			const Protocol::ItemTemplateInfo* itemData = DataManager::Instance()->GetItemTemplate(item.templateid());
			if (itemData)
			{
				stat->set_maxhp(stat->maxhp() + itemData->hpbonus());
				stat->set_attack(stat->attack() + itemData->attackbonus());
				stat->set_defense(stat->defense() + itemData->defensebonus());
			}
		}
	}

	// 3. 체력 보정
	if (stat->hp() > stat->maxhp())
		stat->set_hp(stat->maxhp());

	// [Log]
	// std::cout << "💪 [Stat Refresh] HP:" << stat->maxhp() << " ATK:" << stat->attack() << " DEF:" << stat->defense() << std::endl;
}

void Player::Update()
{
	// TODO: 쿨타임 계산, 버프 처리 등 프레임 단위 로직
}