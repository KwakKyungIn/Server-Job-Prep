pushd %~dp0

:: =============================================================
:: 1. Protoc 컴파일 (Proto -> C++ & C#)
:: =============================================================

:: [C++] 기존 로직
protoc.exe -I=./ --cpp_out=./ ./Enum.proto
protoc.exe -I=./ --cpp_out=./ ./Struct.proto
protoc.exe -I=./ --cpp_out=./ ./Protocol.proto
protoc.exe -I=./ --cpp_out=./ ./Protocol_S2S.proto

:: [C#] 유니티용 로직 (GIGACHAD ADD)
:: S2S는 클라에서 안 쓰지만, Enum이나 Struct 의존성 때문에 같이 뽑는 게 안전함.
protoc.exe -I=./ --csharp_out=./ ./Enum.proto
protoc.exe -I=./ --csharp_out=./ ./Struct.proto
protoc.exe -I=./ --csharp_out=./ ./Protocol.proto

:: =============================================================
:: 2. 패킷 핸들러 생성 (Role-Based)
:: =============================================================

:: [GameServer용]
GenPackets.exe --path=./Protocol.proto --output=ClientPacketHandler --recv=C_ --send=S_ --id=1000

:: [Client용 - C++] (DummyClient용)
GenPackets.exe --path=./Protocol.proto --output=ServerPacketHandler --recv=S_ --send=C_ --id=1000

:: [DBAgent용]
GenPackets.exe --path=./Protocol_S2S.proto --output=DBAgentPacketHandler --recv=S2S_REQ --send=S2S_RES --id=2000

:: [ChatServer용]
GenPackets.exe --path=./Protocol_S2S.proto --output=ChatServerPacketHandler --recv=S2S_REQ --send=S2S_RES --id=2000

:: [GameServer/ChatServer용] S2S
GenPackets.exe --path=./Protocol_S2S.proto --output=S2SPacketHandler --recv=S2S_RES --send=S2S_REQ --id=2000

:: [Client용 - C#] (Unity용) - *추후 Python 스크립트 수정 후 활성화*
:: GenPackets.exe --path=./Protocol.proto --output=PacketHandler --recv=S_ --send=C_ --id=1000 --lang=csharp

IF ERRORLEVEL 1 PAUSE

:: =============================================================
:: 3. 파일 복사 (Deployment)
:: =============================================================

:: --- GameServer ---
XCOPY /Y Enum.pb.h "../../../GameServer"
XCOPY /Y Enum.pb.cc "../../../GameServer"
XCOPY /Y Struct.pb.h "../../../GameServer"
XCOPY /Y Struct.pb.cc "../../../GameServer"
XCOPY /Y Protocol.pb.h "../../../GameServer"
XCOPY /Y Protocol.pb.cc "../../../GameServer"
XCOPY /Y ClientPacketHandler.h "../../../GameServer"
XCOPY /Y Protocol_S2S.pb.h "../../../GameServer"
XCOPY /Y Protocol_S2S.pb.cc "../../../GameServer"
XCOPY /Y S2SPacketHandler.h "../../../GameServer"

:: --- ChatServer ---
XCOPY /Y Enum.pb.h "../../../ChatServer"
XCOPY /Y Enum.pb.cc "../../../ChatServer"
XCOPY /Y Struct.pb.h "../../../ChatServer"
XCOPY /Y Struct.pb.cc "../../../ChatServer"
XCOPY /Y Protocol_S2S.pb.h "../../../ChatServer"
XCOPY /Y Protocol_S2S.pb.cc "../../../ChatServer"
XCOPY /Y ChatServerPacketHandler.h "../../../ChatServer"
XCOPY /Y S2SPacketHandler.h "../../../ChatServer"

:: --- DBAgent ---
XCOPY /Y Enum.pb.h "../../../DBAgent"
XCOPY /Y Enum.pb.cc "../../../DBAgent"
XCOPY /Y Struct.pb.h "../../../DBAgent"
XCOPY /Y Struct.pb.cc "../../../DBAgent"
XCOPY /Y Protocol_S2S.pb.h "../../../DBAgent"
XCOPY /Y Protocol_S2S.pb.cc "../../../DBAgent"
XCOPY /Y DBAgentPacketHandler.h "../../../DBAgent"

:: --- DummyClient (C++) ---
XCOPY /Y Enum.pb.h "../../../DummyClient"
XCOPY /Y Enum.pb.cc "../../../DummyClient"
XCOPY /Y Struct.pb.h "../../../DummyClient"
XCOPY /Y Struct.pb.cc "../../../DummyClient"
XCOPY /Y Protocol.pb.h "../../../DummyClient"
XCOPY /Y Protocol.pb.cc "../../../DummyClient"
XCOPY /Y ServerPacketHandler.h "../../../DummyClient"

:: --- Unity Client (C#) ---
:: [중요] 유니티 프로젝트 경로가 확정되면 아래 경로를 수정해서 주석 해제할 것
:: 현재는 일단 생성된 파일 확인용으로 둠
:: SET UNITY_PATH="../../../NexusClient/Assets/Scripts/Packet"
:: IF NOT EXIST %UNITY_PATH% MKDIR %UNITY_PATH%
:: XCOPY /Y Enum.cs %UNITY_PATH%
:: XCOPY /Y Struct.cs %UNITY_PATH%
:: XCOPY /Y Protocol.cs %UNITY_PATH%

:: =============================================================
:: 4. 청소
:: =============================================================
DEL /Q /F *.pb.h
DEL /Q /F *.pb.cc
DEL /Q /F *.h
:: C# 파일은 유니티로 복사 후 지우는 게 정석이지만, 
:: 지금은 복사 경로가 주석 처리되어 있으니 확인을 위해 남겨두거나 지우거나 선택해라.
:: 일단은 지우지 않고 남겨둔다. (확인용)
:: DEL /Q /F *.cs

PAUSE