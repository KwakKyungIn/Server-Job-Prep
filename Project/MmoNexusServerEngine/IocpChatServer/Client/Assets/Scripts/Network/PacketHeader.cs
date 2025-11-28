using System;
using System.Runtime.InteropServices;

namespace Packet
{
    // [GIGACHAD] C++ 서버와 메모리 구조 일치 (12 Bytes)
    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    public struct PacketHeader
    {
        public ushort size;
        public ushort id;
        public uint crc; // Week 3 Added
        public uint seq; // Week 3 Added
    }
}