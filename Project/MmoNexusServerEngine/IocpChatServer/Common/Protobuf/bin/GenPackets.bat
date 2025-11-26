pushd %~dp0

:: 1. Protoc 컴파일
protoc.exe -I=./ --cpp_out=./ ./Enum.proto
protoc.exe -I=./ --cpp_out=./ ./Struct.proto
protoc.exe -I=./ --cpp_out=./ ./Protocol.proto
protoc.exe -I=./ --cpp_out=./ ./Protocol_S2S.proto

:: 2. 패킷 핸들러 생성 (Role-Based)

:: [GameServer용] 클라에서 온 요청(C_) 처리, 클라로 보낼 응답(S_) 생성
GenPackets.exe --path=./Protocol.proto --output=ClientPacketHandler --recv=C_ --send=S_ --id=1000

:: [Client용] 서버에서 온 응답(S_) 처리, 서버로 보낼 요청(C_) 생성
GenPackets.exe --path=./Protocol.proto --output=ServerPacketHandler --recv=S_ --send=C_ --id=1000

:: [DBAgent용] S2S 요청(REQ) 처리, 응답(RES) 생성
GenPackets.exe --path=./Protocol_S2S.proto --output=DBAgentPacketHandler --recv=S2S_REQ --send=S2S_RES --id=2000

:: [ChatServer용] S2S 요청(REQ) 처리, 응답(RES) 생성 (DBAgent와 역할 동일)
GenPackets.exe --path=./Protocol_S2S.proto --output=ChatServerPacketHandler --recv=S2S_REQ --send=S2S_RES --id=2000

:: [GameServer/ChatServer용] S2S 응답(RES) 처리, 요청(REQ) 생성
:: (GameServer가 DB/Chat에 요청 보낼 때 사용)
GenPackets.exe --path=./Protocol_S2S.proto --output=S2SPacketHandler --recv=S2S_RES --send=S2S_REQ --id=2000

IF ERRORLEVEL 1 PAUSE

:: 3. 파일 복사 (Deployment)

:: --- GameServer (Main Hub) ---
XCOPY /Y Enum.pb.h "../../../GameServer"
XCOPY /Y Enum.pb.cc "../../../GameServer"
XCOPY /Y Struct.pb.h "../../../GameServer"
XCOPY /Y Struct.pb.cc "../../../GameServer"
XCOPY /Y Protocol.pb.h "../../../GameServer"
XCOPY /Y Protocol.pb.cc "../../../GameServer"
XCOPY /Y ClientPacketHandler.h "../../../GameServer"
:: S2S (DB/Chat 접속용)
XCOPY /Y Protocol_S2S.pb.h "../../../GameServer"
XCOPY /Y Protocol_S2S.pb.cc "../../../GameServer"
XCOPY /Y S2SPacketHandler.h "../../../GameServer"

:: --- ChatServer (Sub Server) ---
XCOPY /Y Enum.pb.h "../../../ChatServer"
XCOPY /Y Enum.pb.cc "../../../ChatServer"
XCOPY /Y Struct.pb.h "../../../ChatServer"
XCOPY /Y Struct.pb.cc "../../../ChatServer"
:: S2S (GameServer 접속 받기용)
XCOPY /Y Protocol_S2S.pb.h "../../../ChatServer"
XCOPY /Y Protocol_S2S.pb.cc "../../../ChatServer"
XCOPY /Y ChatServerPacketHandler.h "../../../ChatServer"
:: (만약 ChatServer도 DB에 붙는다면 S2SPacketHandler도 필요함. 일단 복사해둠)
XCOPY /Y S2SPacketHandler.h "../../../ChatServer"

:: --- DBAgent (DB Server) ---
XCOPY /Y Enum.pb.h "../../../DBAgent"
XCOPY /Y Enum.pb.cc "../../../DBAgent"
XCOPY /Y Struct.pb.h "../../../DBAgent"
XCOPY /Y Struct.pb.cc "../../../DBAgent"
:: S2S
XCOPY /Y Protocol_S2S.pb.h "../../../DBAgent"
XCOPY /Y Protocol_S2S.pb.cc "../../../DBAgent"
XCOPY /Y DBAgentPacketHandler.h "../../../DBAgent"

:: --- DummyClient (Test Client) ---
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