using System;

public class Crc32
{
    static uint[] _table;

    static Crc32()
    {
        uint poly = 0xEDB88320;
        _table = new uint[256];
        uint temp = 0;
        for (uint i = 0; i < _table.Length; ++i)
        {
            temp = i;
            for (int j = 8; j > 0; --j)
            {
                if ((temp & 1) == 1)
                    temp = (uint)((temp >> 1) ^ poly);
                else
                    temp >>= 1;
            }
            _table[i] = temp;
        }
    }

    public static uint Compute(byte[] bytes, int offset, int count)
    {
        uint crc = 0xffffffff;
        for (int i = 0; i < count; ++i)
        {
            byte index = (byte)(((crc) & 0xff) ^ bytes[offset + i]);
            crc = (uint)((crc >> 8) ^ _table[index]);
        }
        return ~crc;
    }
}