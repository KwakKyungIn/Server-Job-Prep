pushd %~dp0

:: =============================================================
:: 1. Protoc Compile (Proto -> C++ & C#)
:: =============================================================

:: [C++]
protoc.exe -I=./ --cpp_out=./ ./Enum.proto
protoc.exe -I=./ --cpp_out=./ ./Struct.proto
protoc.exe -I=./ --cpp_out=./ ./Protocol.proto
protoc.exe -I=./ --cpp_out=./ ./Protocol_S2S.proto

:: [C#] (For Unity)
protoc.exe -I=./ --csharp_out=./ ./Enum.proto
protoc.exe -I=./ --csharp_out=./ ./Struct.proto
protoc.exe -I=./ --csharp_out=./ ./Protocol.proto

:: =============================================================
:: 2. Packet Handler Generation (Role-Based)
:: =============================================================

:: [GameServer]
GenPackets.exe --path=./Protocol.proto --output=ClientPacketHandler --recv=C_ --send=S_ --id=1000

:: [Client - C++] (DummyClient)
GenPackets.exe --path=./Protocol.proto --output=ServerPacketHandler --recv=S_ --send=C_ --id=1000

:: [DBAgent]
GenPackets.exe --path=./Protocol_S2S.proto --output=DBAgentPacketHandler --recv=S2S_REQ --send=S2S_RES --id=2000

:: [ChatServer]
GenPackets.exe --path=./Protocol_S2S.proto --output=ChatServerPacketHandler --recv=S2S_REQ --send=S2S_RES --id=2000

:: [S2S Common]
GenPackets.exe --path=./Protocol_S2S.proto --output=S2SPacketHandler --recv=S2S_RES --send=S2S_REQ --id=2000

:: [Client - C#] (For Unity - PacketManager)
GenPackets.exe --path=./Protocol.proto --output=PacketManager --recv=S_ --send=C_ --id=1000 --lang=csharp

IF ERRORLEVEL 1 PAUSE

:: =============================================================
:: 3. File Copy (Deployment)
:: =============================================================

:: --- GameServer ---
XCOPY /Y Enum.pb.h "..\..\..\GameServer"
XCOPY /Y Enum.pb.cc "..\..\..\GameServer"
XCOPY /Y Struct.pb.h "..\..\..\GameServer"
XCOPY /Y Struct.pb.cc "..\..\..\GameServer"
XCOPY /Y Protocol.pb.h "..\..\..\GameServer"
XCOPY /Y Protocol.pb.cc "..\..\..\GameServer"
XCOPY /Y ClientPacketHandler.h "..\..\..\GameServer"
XCOPY /Y Protocol_S2S.pb.h "..\..\..\GameServer"
XCOPY /Y Protocol_S2S.pb.cc "..\..\..\GameServer"
XCOPY /Y S2SPacketHandler.h "..\..\..\GameServer"

:: --- LoginServer (NEW) ---
XCOPY /Y Enum.pb.h "..\..\..\LoginServer"
XCOPY /Y Enum.pb.cc "..\..\..\LoginServer"
XCOPY /Y Struct.pb.h "..\..\..\LoginServer"
XCOPY /Y Struct.pb.cc "..\..\..\LoginServer"
XCOPY /Y Protocol.pb.h "..\..\..\LoginServer"
XCOPY /Y Protocol.pb.cc "..\..\..\LoginServer"
XCOPY /Y ClientPacketHandler.h "..\..\..\LoginServer"
XCOPY /Y Protocol_S2S.pb.h "..\..\..\LoginServer"
XCOPY /Y Protocol_S2S.pb.cc "..\..\..\LoginServer"
XCOPY /Y S2SPacketHandler.h "..\..\..\LoginServer"

:: --- ChatServer ---
XCOPY /Y Enum.pb.h "..\..\..\ChatServer"
XCOPY /Y Enum.pb.cc "..\..\..\ChatServer"
XCOPY /Y Struct.pb.h "..\..\..\ChatServer"
XCOPY /Y Struct.pb.cc "..\..\..\ChatServer"
XCOPY /Y Protocol_S2S.pb.h "..\..\..\ChatServer"
XCOPY /Y Protocol_S2S.pb.cc "..\..\..\ChatServer"
XCOPY /Y ChatServerPacketHandler.h "..\..\..\ChatServer"
XCOPY /Y S2SPacketHandler.h "..\..\..\ChatServer"
XCOPY /Y Protocol.pb.h "..\..\..\ChatServer"
XCOPY /Y Protocol.pb.cc "..\..\..\ChatServer"


:: --- DBAgent ---
XCOPY /Y Enum.pb.h "..\..\..\DBAgent"
XCOPY /Y Enum.pb.cc "..\..\..\DBAgent"
XCOPY /Y Struct.pb.h "..\..\..\DBAgent"
XCOPY /Y Struct.pb.cc "..\..\..\DBAgent"
XCOPY /Y Protocol_S2S.pb.h "..\..\..\DBAgent"
XCOPY /Y Protocol_S2S.pb.cc "..\..\..\DBAgent"
XCOPY /Y DBAgentPacketHandler.h "..\..\..\DBAgent"
XCOPY /Y Protocol.pb.h "..\..\..\DBAgent"
XCOPY /Y Protocol.pb.cc "..\..\..\DBAgent"


:: --- DummyClient (C++) ---
XCOPY /Y Enum.pb.h "..\..\..\DummyClient"
XCOPY /Y Enum.pb.cc "..\..\..\DummyClient"
XCOPY /Y Struct.pb.h "..\..\..\DummyClient"
XCOPY /Y Struct.pb.cc "..\..\..\DummyClient"
XCOPY /Y Protocol.pb.h "..\..\..\DummyClient"
XCOPY /Y Protocol.pb.cc "..\..\..\DummyClient"
XCOPY /Y ServerPacketHandler.h "..\..\..\DummyClient"

:: --- Unity Client (C#) ---
SET UNITY_PATH=..\..\..\Client\Assets\Scripts\Packet

IF NOT EXIST %UNITY_PATH% MKDIR %UNITY_PATH%

XCOPY /Y Enum.cs %UNITY_PATH%
XCOPY /Y Struct.cs %UNITY_PATH%
XCOPY /Y Protocol.cs %UNITY_PATH%
XCOPY /Y PacketManager.cs %UNITY_PATH%

:: =============================================================
:: 4. Cleanup
:: =============================================================
DEL /Q /F *.pb.h
DEL /Q /F *.pb.cc
DEL /Q /F *.h
DEL /Q /F *.cs

PAUSE