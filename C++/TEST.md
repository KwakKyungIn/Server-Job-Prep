sequenceDiagram
autonumber
actor C as Client (ServerSession)
participant NET as TCP/IP
participant L as Listener
participant IOCP as IocpCore
participant SV as ServerService
participant S as ChatSession
participant SM as ChatSessionManager
participant H as ClientPacketHandler
participant POOL as DBConnectionPool
participant CONN as DBConnection
participant SQL as MSSQL (Players)


C->>NET: TCP Connect (SYN)
NET-->>L: AcceptEx Overlapped
L->>SV: ProcessAccept (attach socket)
SV->>S: CreateSession(ChatSession)
IOCP-->>S: Connect completed (Event)
activate S
S->>SV: Session::ProcessConnect → AddSession()
S->>SM: OnConnected() → Add(session)
S-->>IOCP: RegisterRecv()
deactivate S


C->>S: C_LOGIN_REQ(name)
activate S
S->>H: OnRecvPacket → Handle_C_LOGIN_REQ
H->>POOL: Pop()
POOL-->>H: DBConnection*
activate CONN
H->>CONN: Prepare/Bind/Execute (SELECT playerId FROM Players WHERE name=?)
CONN->>SQL: Query
SQL-->>CONN: Row(playerId)
CONN-->>H: BindCol(1), Fetch()
H->>POOL: Unbind(); Push(conn)
deactivate CONN


alt success (player found)
H->>S: chatSession.SetPlayer(new Player{name,id})
H-->>C: S_LOGIN_RES(CONNECT_OK, playerId)
else fail (not found/DB error)
H-->>C: S_LOGIN_RES(CONNECT_FAIL, reason)
end


deactivate S
