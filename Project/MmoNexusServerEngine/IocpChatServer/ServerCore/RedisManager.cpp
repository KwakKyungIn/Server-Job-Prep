#include "pch.h"
#include "RedisManager.h"
#include <iostream>

RedisManager::RedisManager()
{
    // 윈도우 소켓 초기화는 CoreGlobal에서 이미 했으니 통과
}

RedisManager::~RedisManager()
{
    std::lock_guard<std::mutex> lock(_mx);
    if (_client.is_connected())
        _client.disconnect();
}

bool RedisManager::Connect(const std::string& ip, int32 port, const std::string& password)
{
    try
    {
        std::lock_guard<std::mutex> lock(_mx);

        _client.connect(ip, port,
            [](const std::string& host, std::size_t port, cpp_redis::client::connect_state status)
            {
                if (status == cpp_redis::client::connect_state::dropped)
                    std::cout << " [Redis] Connection Dropped!" << std::endl;
            });

        if (!password.empty())
        {
            // 필요하면 사용
            // auto f = _client.auth(password);
            // _client.sync_commit();
            // auto r = f.get();
            // if (r.is_error()) return false;
        }

        std::cout << " [Redis] Connected to " << ip << ":" << port << std::endl;
        return true;
    }
    catch (const std::exception& e)
    {
        std::cout << " [Redis] Connection Failed: " << e.what() << std::endl;
        return false;
    }
}

bool RedisManager::Set(const std::string& key, const std::string& value, int32 expireSeconds)
{
    try
    {
        std::lock_guard<std::mutex> lock(_mx);

        auto future = _client.set(key, value);
        _client.sync_commit();

        auto reply = future.get();
        if (reply.is_error())
        {
            std::cout << " [Redis] Set Failed: " << reply.error() << std::endl;
            return false;
        }

        if (expireSeconds > 0)
        {
            _client.expire(key, expireSeconds);
            _client.sync_commit();
        }

        return true;
    }
    catch (const std::exception& e)
    {
        std::cout << " [Redis] Set Exception: " << e.what() << std::endl;
        return false;
    }
}

std::string RedisManager::Get(const std::string& key)
{
    try
    {
        std::lock_guard<std::mutex> lock(_mx);

        auto future = _client.get(key);
        _client.sync_commit();

        auto reply = future.get();
        if (reply.is_null())
            return "";

        if (reply.is_error())
            return "";

        return reply.as_string();
    }
    catch (const std::exception& e)
    {
        std::cout << " [Redis] Get Exception: " << e.what() << std::endl;
        return "";
    }
}

bool RedisManager::Delete(const std::string& key)
{
    try
    {
        std::lock_guard<std::mutex> lock(_mx);

        auto future = _client.del({ key });
        _client.sync_commit();

        auto reply = future.get();
        if (reply.is_error())
            return false;

        return true;
    }
    catch (...)
    {
        return false;
    }
}

// ===================== Hash =====================

bool RedisManager::HSet(const std::string& key, const std::string& field, const std::string& value)
{
    try
    {
        std::lock_guard<std::mutex> lock(_mx);

        auto future = _client.hset(key, field, value);
        _client.sync_commit();

        auto reply = future.get();
        if (reply.is_error())
            return false;

        return true;
    }
    catch (...) { return false; }
}

bool RedisManager::HDel(const std::string& key, const std::string& field)
{
    try
    {
        std::lock_guard<std::mutex> lock(_mx);

        auto future = _client.hdel(key, { field });
        _client.sync_commit();

        auto reply = future.get();
        if (reply.is_error())
            return false;

        return true;
    }
    catch (...) { return false; }
}

bool RedisManager::HGetAll(const std::string& key, std::unordered_map<std::string, std::string>& out)
{
    out.clear();

    try
    {
        std::lock_guard<std::mutex> lock(_mx);

        auto future = _client.hgetall(key);
        _client.sync_commit();

        auto reply = future.get();
        if (reply.is_error())
            return false;

        if (reply.is_null())
            return true; // 키가 없으면 빈 맵으로 OK

        auto arr = reply.as_array();
        if (arr.size() % 2 != 0)
            return false;

        for (size_t i = 0; i < arr.size(); i += 2)
        {
            const auto& f = arr[i];
            const auto& v = arr[i + 1];
            if (!f.is_string() || !v.is_string())
                continue;

            out[f.as_string()] = v.as_string();
        }

        return true;
    }
    catch (...) { return false; }
}

// ===================== Set =====================

bool RedisManager::SAdd(const std::string& key, const std::string& member)
{
    try
    {
        std::lock_guard<std::mutex> lock(_mx);

        auto future = _client.sadd(key, { member });
        _client.sync_commit();

        auto reply = future.get();
        if (reply.is_error())
            return false;

        return true;
    }
    catch (...) { return false; }
}

bool RedisManager::SRem(const std::string& key, const std::string& member)
{
    try
    {
        std::lock_guard<std::mutex> lock(_mx);

        auto future = _client.srem(key, { member });
        _client.sync_commit();

        auto reply = future.get();
        if (reply.is_error())
            return false;

        return true;
    }
    catch (...) { return false; }
}

bool RedisManager::SMembers(const std::string& key, std::vector<std::string>& out)
{
    out.clear();

    try
    {
        std::lock_guard<std::mutex> lock(_mx);

        auto future = _client.smembers(key);
        _client.sync_commit();

        auto reply = future.get();
        if (reply.is_error())
            return false;

        if (reply.is_null())
            return true;

        auto arr = reply.as_array();
        out.reserve(arr.size());

        for (const auto& e : arr)
        {
            if (!e.is_string())
                continue;
            out.push_back(e.as_string());
        }

        return true;
    }
    catch (...) { return false; }
}
