#include "pch.h"
#include "Crc32.h"

uint32 Crc32::_table[256];
bool Crc32::_init = false;

void Crc32::Init()
{
	if (_init)
		return;

	const uint32 poly = 0xEDB88320; // CRC-32-IEEE 802.3 Polynomial

	for (uint32 i = 0; i < 256; i++)
	{
		uint32 crc = i;
		for (uint32 j = 0; j < 8; j++)
		{
			if (crc & 1)
				crc = (crc >> 1) ^ poly;
			else
				crc >>= 1;
		}
		_table[i] = crc;
	}

	_init = true;
}

uint32 Crc32::Compute(BYTE* buffer, int32 len)
{
	if (_init == false)
		Init();

	uint32 crc = 0xFFFFFFFF; // Initial Value

	for (int32 i = 0; i < len; i++)
	{
		const uint8 index = (crc ^ buffer[i]) & 0xFF;
		crc = (crc >> 8) ^ _table[index];
	}

	return ~crc; // Final XOR
}