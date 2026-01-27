#pragma once
#include "GameRoom.h"
#include "LobbyRoom.h"

// 맵을 구분하는 유니크 키
// Map의 Key로 쓰려면 정렬 기준(operator<)이 필요해서 구조체로 뺌
struct RoomKey
{
    int32 channelId;
    int32 mapId;
    int64 instanceId = 0; // 0이면 일반 필드, 그 외엔 인스턴스 던전 ID

    // std::map이나 std::set에서 키로 쓰려면 비교 연산자가 필수임
    bool operator<(const RoomKey& other) const
    {
        if (channelId != other.channelId) return channelId < other.channelId;
        if (mapId != other.mapId)         return mapId < other.mapId;
        return instanceId < other.instanceId;
    }
};

// 서버 내 모든 방을 관리하는 매니저
// 방 생성, 삭제, 검색을 담당함
class RoomManager
{
public:
    RoomManager() = default;
    ~RoomManager() = default;

    // 일반 필드 / 인스턴스 던전 공용 조회 및 생성
    std::shared_ptr<GameRoom> GetOrCreateRoom(int32 channelId, int32 mapId, int64 instanceId = 0);
    std::shared_ptr<GameRoom> FindRoom(int32 channelId, int32 mapId, int64 instanceId = 0);

    // 로비는 게임 룸과 성격이 달라서 별도 관리
    // 캐릭터 선택이나 월드 진입 전 대기 장소
    std::shared_ptr<LobbyRoom> GetOrCreateLobby(int32 channelId);


    // 메인 루프에서 호출해서 각 방들을 업데이트 시킴
    void UpdateAll();

private:
    // 동시성 제어를 위한 락
    USE_LOCK;
    Map<RoomKey, std::shared_ptr<GameRoom>> _rooms;

    // 빈 인스턴스 방 청소
    void PurgeInstanceRooms(uint64 nowMs);

    // 채널별 로비 목록
    HashMap<int32, std::shared_ptr<LobbyRoom>> _lobbies;

};

// 전역에서 접근하기 쉽게 싱글톤처럼 사용
extern std::shared_ptr<RoomManager> GRoomManager;