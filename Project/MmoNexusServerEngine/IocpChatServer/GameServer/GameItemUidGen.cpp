#include "pch.h"
#include "GameItemUidGen.h"

// 정적 멤버 변수 초기화
// 서버가 실수로 Init 호출 안 했을 때를 대비한 임시 값
// 실제로는 DB에서 Max(UID)를 읽어와서 세팅해야 함
std::atomic<uint64_t> GameItemUidGen::_next{ 1000000 };