#include "pch.h"
#include "RedisManager.h"

RedisManager::RedisManager()
{
    // 윈도우 소켓 초기화는 CoreGlobal에서 이미 했으니 통과
}

RedisManager::~RedisManager()
{
    if (_client.is_connected())
        _client.disconnect();
}

bool RedisManager::Connect(const std::string& ip, int32 port, const std::string& password)
{
    try
    {
        _client.connect(ip, port, [](const std::string& host, std::size_t port, cpp_redis::client::connect_state status) {
            if (status == cpp_redis::client::connect_state::dropped)
            {
                std::cout << "❌ [Redis] Connection Dropped!" << std::endl;
            }
            });

        // 비밀번호가 있다면 인증
        if (password.empty() == false)
        {
            // _client.auth(password); // 필요하면 주석 해제
        }

        std::cout << "✅ [Redis] Connected to " << ip << ":" << port << std::endl;
        return true;
    }
    catch (const std::exception& e)
    {
        std::cout << "❌ [Redis] Connection Failed: " << e.what() << std::endl;
        return false;
    }
}

bool RedisManager::Set(const std::string& key, const std::string& value, int32 expireSeconds)
{
    try
    {
        // 값을 저장 (Set)
        auto future = _client.set(key, value);
        _client.sync_commit(); // 커밋해야 전송됨

        // 결과 대기 (Blocking)
        auto reply = future.get();
        if (reply.is_error())
        {
            std::cout << "❌ [Redis] Set Failed: " << reply.error() << std::endl;
            return false;
        }

        // 만료 시간 설정 (TTL) - 토큰은 영원하지 않다.
        if (expireSeconds > 0)
        {
            _client.expire(key, expireSeconds);
            _client.sync_commit();
        }

        return true;
    }
    catch (const std::exception& e)
    {
        std::cout << "❌ [Redis] Set Exception: " << e.what() << std::endl;
        return false;
    }
}

std::string RedisManager::Get(const std::string& key)
{
    try
    {
        auto future = _client.get(key);
        _client.sync_commit();

        auto reply = future.get();
        if (reply.is_null()) // 키가 없음
            return "";

        return reply.as_string();
    }
    catch (const std::exception& e)
    {
        std::cout << "❌ [Redis] Get Exception: " << e.what() << std::endl;
        return "";
    }
}

bool RedisManager::Delete(const std::string& key)
{
    try
    {
        auto future = _client.del({ key });
        _client.sync_commit();
        future.wait(); // 결과 대기
        return true;
    }
    catch (const std::exception& e)
    {
        return false;
    }
}