#pragma once

// [GIGACHAD] CRC32 Calculator
// Lookup Table 방식을 사용하여 매우 빠르다.
class Crc32
{
public:
	static void Init();
	static uint32 Compute(BYTE* buffer, int32 len);

private:
	static uint32 _table[256];
	static bool _init;
};