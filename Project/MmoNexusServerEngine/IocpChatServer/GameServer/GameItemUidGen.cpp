#include "pch.h"
#include "GameItemUidGen.h"

// fallback 값(Init 안 됐을 때 임시) 
std::atomic<uint64_t> GameItemUidGen::_next{ 1000000 };
