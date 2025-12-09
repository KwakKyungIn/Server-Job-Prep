#pragma once
#include <cpp_redis/cpp_redis>

/*
    [GIGACHAD NOTE]
    Redis는 열쇠고리(Key-Value) 저장소다.
    LoginServer가 토큰을 발행해서 저장하고,
    GameServer가 그 토큰이 진짜인지 검사할 때 쓴다.
*/

class RedisManager
{
public:
    RedisManager();
    ~RedisManager();

    // 연결 및 초기화
    bool Connect(const std::string& ip, int32 port, const std::string& password = "");

    // [동기 방식] 값을 쓰고/읽기 (Blocking)
    // 초반엔 복잡한 비동기(Async)보다 이게 정신건강에 좋다.
    bool Set(const std::string& key, const std::string& value, int32 expireSeconds = 0);
    std::string Get(const std::string& key);
    bool Delete(const std::string& key);

private:
    cpp_redis::client _client;
};