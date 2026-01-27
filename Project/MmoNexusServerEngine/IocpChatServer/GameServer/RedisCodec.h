#pragma once

namespace Persistence
{
    // [경량화 직렬화 모듈]
    // Protobuf나 JSON은 오버헤드가 좀 있어서, 간단한 구조체는 직접 구분자(|)를 써서 문자열로 만듦
    // Redis Hash 필드 값으로 들어갈 예정

    // 포맷: templateId|slot|count|equipped(0 or 1)
    std::string PackItem(int32 templateId, int32 slot, int32 count, bool equipped);

    // 포맷: refType|refId
    std::string PackQuick(int32 refType, uint64 refId);
    bool UnpackQuick(const std::string& s, int32& outRefType, uint64& outRefId);

    // 파싱 실패 시 false 반환 (데이터 오염 방지)
    bool UnpackItem(const std::string& s,
        int32& outTemplateId,
        int32& outSlot,
        int32& outCount,
        bool& outEquipped);
}