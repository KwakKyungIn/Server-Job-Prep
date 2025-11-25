pushd %~dp0

:: 1. Protoc 컴파일
protoc.exe -I=./ --cpp_out=./ ./Enum.proto
protoc.exe -I=./ --cpp_out=./ ./Struct.proto
protoc.exe -I=./ --cpp_out=./ ./Protocol.proto
protoc.exe -I=./ --cpp_out=./ ./Protocol_S2S.proto

:: 2. 파이썬 생성기 호출 (핸들러 생성)

:: [주의] 곽삣삐의 네이밍 철학 반영 (Source-based Naming)

:: (1) ClientPacketHandler -> "클라이언트로부터 온 패킷을 처리"
::     위치: Server / 역할: Recv C_, Send S_
GenPackets.exe --path=./Protocol.proto --output=ClientPacketHandler --recv=C_ --send=S_ --id=1000

:: (2) ServerPacketHandler -> "서버로부터 온 패킷을 처리"
::     위치: Client / 역할: Recv S_, Send C_
GenPackets.exe --path=./Protocol.proto --output=ServerPacketHandler --recv=S_ --send=C_ --id=1000

:: (3) DBAgentPacketHandler -> "DBAgent가 처리할 패킷 (Game -> DB)"
::     위치: DBAgent / 역할: Recv S2S_REQ, Send S2S_RES
GenPackets.exe --path=./Protocol_S2S.proto --output=DBAgentPacketHandler --recv=S2S_REQ --send=S2S_RES --id=2000

:: (4) S2SPacketHandler -> "GameServer가 처리할 패킷 (DB -> Game)"
::     위치: Game/ChatServer / 역할: Recv S2S_RES, Send S2S_REQ
GenPackets.exe --path=./Protocol_S2S.proto --output=S2SPacketHandler --recv=S2S_RES --send=S2S_REQ --id=2000

IF ERRORLEVEL 1 PAUSE

:: 3. 파일 복사 (Deployment)
:: 네가 원하는 위치 그대로 유지

:: --- ChatServer (Server App) ---
:: 서버니까 ClientPacketHandler(C를 처리하는 놈)를 가져감 -> Correct
XCOPY /Y Enum.pb.h "../../../ChatServer"
XCOPY /Y Enum.pb.cc "../../../ChatServer"
XCOPY /Y Struct.pb.h "../../../ChatServer"
XCOPY /Y Struct.pb.cc "../../../ChatServer"
XCOPY /Y Protocol.pb.h "../../../ChatServer"
XCOPY /Y Protocol.pb.cc "../../../ChatServer"
XCOPY /Y ClientPacketHandler.h "../../../ChatServer"
:: S2S
XCOPY /Y Protocol_S2S.pb.h "../../../ChatServer"
XCOPY /Y Protocol_S2S.pb.cc "../../../ChatServer"
XCOPY /Y S2SPacketHandler.h "../../../ChatServer"

:: --- DBAgent (DB App) ---
XCOPY /Y Enum.pb.h "../../../DBAgent"
XCOPY /Y Enum.pb.cc "../../../DBAgent"
XCOPY /Y Struct.pb.h "../../../DBAgent"
XCOPY /Y Struct.pb.cc "../../../DBAgent"
XCOPY /Y Protocol_S2S.pb.h "../../../DBAgent"
XCOPY /Y Protocol_S2S.pb.cc "../../../DBAgent"
XCOPY /Y DBAgentPacketHandler.h "../../../DBAgent"

:: --- DummyClient (Client App) ---
:: 클라니까 ServerPacketHandler(S를 처리하는 놈)를 가져감 -> Correct
XCOPY /Y Enum.pb.h "../../../DummyClient"
XCOPY /Y Enum.pb.cc "../../../DummyClient"
XCOPY /Y Struct.pb.h "../../../DummyClient"
XCOPY /Y Struct.pb.cc "../../../DummyClient"
XCOPY /Y Protocol.pb.h "../../../DummyClient"
XCOPY /Y Protocol.pb.cc "../../../DummyClient"
XCOPY /Y ServerPacketHandler.h "../../../DummyClient"

:: 4. 청소
DEL /Q /F *.pb.h
DEL /Q /F *.pb.cc
DEL /Q /F *.h

PAUSE