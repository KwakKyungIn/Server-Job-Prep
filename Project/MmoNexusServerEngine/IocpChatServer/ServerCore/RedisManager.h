#pragma once
#include <cpp_redis/cpp_redis>
#include <mutex>
#include <unordered_map>
#include <vector>

/*
    [GIGACHAD NOTE]
    Redis는 열쇠고리(Key-Value) 저장소다.
    LoginServer가 토큰을 발행해서 저장하고,
    GameServer가 그 토큰이 진짜인지 검사할 때 쓴다.

    [A UPDATE]
    이제는 Write-Back 저장소로도 쓴다:
    - HASH: p:{pid}:core, p:{pid}:inv
    - SET : dirty:player, dirty:inv, p:{pid}:invdel
*/

class RedisManager
{
public:
    RedisManager();
    ~RedisManager();

    // 연결 및 초기화
    bool Connect(const std::string& ip, int32 port, const std::string& password = "");

    // ===== KV (Blocking) =====
    bool Set(const std::string& key, const std::string& value, int32 expireSeconds = 0);
    std::string Get(const std::string& key);
    bool Delete(const std::string& key);

    // ===== Hash =====
    bool HSet(const std::string& key, const std::string& field, const std::string& value);
    bool HDel(const std::string& key, const std::string& field);
    bool HGetAll(const std::string& key, std::unordered_map<std::string, std::string>& out);

    // ===== Set =====
    bool SAdd(const std::string& key, const std::string& member);
    bool SRem(const std::string& key, const std::string& member);
    bool SMembers(const std::string& key, std::vector<std::string>& out);

    // ===== Key =====
    bool Del(const std::string& key) { return Delete(key); }

private:
    // cpp_redis client는 sync_commit 기반에서 멀티스레드 안전 보장 안 한다고 보고
    // 우리 쪽에서 직렬화한다.
    std::mutex _mx;
    cpp_redis::client _client;
};
