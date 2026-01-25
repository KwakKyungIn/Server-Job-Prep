# Repository Guidelines

## Project Structure & Module Organization
- `ServerCore/`: IOCP networking core, buffers, locks, job queues, memory pools.
- `GameServer/`: main gameplay server (rooms, movement, combat, AI, persistence). Entry: `GameServer.cpp`.
- `LoginServer/`: login/auth server. Entry: `LoginServer.cpp`.
- `DBAgent/`: DB/Redis bridge and S2S handlers. Entry: `DBAgent.cpp`.
- `Common/Protobuf/bin/`: protocol definitions (`Protocol.proto`, `Protocol_S2S.proto`, `Enum.proto`, `Struct.proto`).
- `Tools/PacketGenerator/`: packet handler/codegen scripts and templates.
- `docs/`: portfolio documentation and other notes.
- Top-level solution: `IocpChatServer.sln`.

## Build, Test, and Development Commands
- Build (Visual Studio): open `IocpChatServer.sln`, build `x64` + `Debug/Release`.
- Build (CLI, Windows):
  ```bat
  msbuild IocpChatServer.sln /p:Configuration=Debug /p:Platform=x64
  ```
- Run locally (typical order): start `LoginServer`, `DBAgent`, then `GameServer`.
- Packet codegen (if updating .proto):
  ```bash
  python Tools/PacketGenerator/PacketGenerator.py --path Common/Protobuf/bin/Protocol.proto --output ClientPacketHandler
  ```

## Coding Style & Naming Conventions
- Language: C++ (MSVC). Indentation appears to be tabs with 4-width display in server code.
- Files use `.cpp/.h` pairs; classes are `PascalCase`, methods `PascalCase`, locals `camelCase`.
- Prefer existing patterns: `Handle_C_*` / `S2S_REQ_*` naming in packet handlers.
- Formatting: no automated formatter found; follow surrounding style in file.

## Testing Guidelines
- No automated test framework found in repo.
- Prefer manual verification via running servers and observing logs.
- When changing protocol or gameplay flow, validate:
  - login → enter game → spawn
  - movement validation (clamp/sliding)
  - combat (skill → damage)
  - persistence (disconnect → auto-commit)

## Commit & Pull Request Guidelines
- Commit messages follow a Conventional Commits style with scopes, e.g.
  `feat(trade): ...`, `fix(client/move): ...`, `perf(core/memory): ...`,
  `refactor(memory): ...`, `docs(nav): ...`, `chore(encoding): ...`.
- Keep summaries concise and imperative; include a focused scope.
- PRs should include: problem statement, scope of change, manual test steps, and any config changes.

## Security & Configuration Tips
- Config file: `ServerConfig.json` (loaded in `ServerCore/CoreGlobal.cpp`).
- Map settings: `GameServer/Maps.json` (loaded by `DataManager`).
- Do not commit secrets in `ServerConfig.json`; prefer local overrides for credentials.

## Architecture Overview
- IOCP-based networking with `Session` → `PacketSession` → server-specific sessions.
- Game logic runs on `JobQueue`/`RoomActor` to reduce contention.
- Persistence uses Redis write-back + periodic DB commit via DBAgent.
