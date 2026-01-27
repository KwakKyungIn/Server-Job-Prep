#include "pch.h"
#include "RedisCodec.h"

namespace Persistence
{
    // 아이템 정보를 하나의 문자열로 압축하는 함수
    // Redis는 문자열 처리에 최적화되어 있어서, 복잡한 객체를 그대로 넣기보다
    // 이렇게 구분자(|)를 써서 직렬화하는 게 메모리나 속도 면에서 훨씬 가볍다
    std::string PackItem(int32 templateId, int32 slot, int32 count, bool equipped)
    {
        return std::to_string(templateId) + "|" +
            std::to_string(slot) + "|" +
            std::to_string(count) + "|" +
            (equipped ? "1" : "0");
    }

    // 문자열을 숫자로 바꿀 때 안전장치
    // stoll은 변환 실패하면 예외를 던지는데, 서버가 죽으면 안 되니까 try-catch로 감싸둠
    static bool ParseI32(const std::string& s, int32& out)
    {
        try
        {
            long long v = std::stoll(s);
            // int32 범위 벗어나는 오버플로우 체크
            if (v < INT32_MIN || v > INT32_MAX) return false;
            out = static_cast<int32>(v);
            return true;
        }
        catch (...) { return false; }
    }

    // Redis에서 읽어온 문자열을 다시 아이템 정보로 분해(파싱)하는 함수
    // 구분자(|) 위치를 찾아서 서브스트링으로 쪼개는 방식
    bool UnpackItem(const std::string& s,
        int32& outTemplateId,
        int32& outSlot,
        int32& outCount,
        bool& outEquipped)
    {
        // 구분자(|)가 3개 있어야 정상적인 포맷임
        size_t p1 = s.find('|');
        if (p1 == std::string::npos) return false;
        size_t p2 = s.find('|', p1 + 1);
        if (p2 == std::string::npos) return false;
        size_t p3 = s.find('|', p2 + 1);
        if (p3 == std::string::npos) return false;

        // 인덱스 기준으로 문자열 자르기
        std::string a = s.substr(0, p1);
        std::string b = s.substr(p1 + 1, p2 - (p1 + 1));
        std::string c = s.substr(p2 + 1, p3 - (p2 + 1));
        std::string d = s.substr(p3 + 1);

        int32 t = 0, slot = 0, cnt = 0;
        // 하나라도 파싱 실패하면 잘못된 데이터로 간주
        if (!ParseI32(a, t)) return false;
        if (!ParseI32(b, slot)) return false;
        if (!ParseI32(c, cnt)) return false;

        // 장착 여부는 0/1로 저장했었음
        if (d == "1") outEquipped = true;
        else if (d == "0") outEquipped = false;
        else return false;

        outTemplateId = t;
        outSlot = slot;
        outCount = cnt;
        return true;
    }

    // 퀵슬롯 정보 직렬화
    // 타입(아이템인지 스킬인지)과 ID만 있으면 됨
    std::string PackQuick(int32 refType, uint64 refId)
    {
        return std::to_string(refType) + "|" + std::to_string(refId);
    }

    // uint64 파싱용 헬퍼 함수
    static bool ParseU64(const std::string& s, uint64& out)
    {
        try { out = (uint64)std::stoull(s); return true; }
        catch (...) { return false; }
    }

    bool UnpackQuick(const std::string& s, int32& outRefType, uint64& outRefId)
    {
        size_t p = s.find('|');
        if (p == std::string::npos) return false;

        std::string a = s.substr(0, p);
        std::string b = s.substr(p + 1);

        int32 rt = 0;
        if (!ParseI32(a, rt)) return false;

        uint64 rid = 0;
        if (!ParseU64(b, rid)) return false;

        outRefType = rt;
        outRefId = rid;
        return true;
    }
}