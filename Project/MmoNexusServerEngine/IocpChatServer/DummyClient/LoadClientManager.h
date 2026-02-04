#pragma once
#include "Protocol.pb.h"
#include "Service.h"
#include "LoadClientConfig.h"

#include <atomic>
#include <fstream>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

class LoadClientManager;
class NavSystem;
class LoadLoginSession;
class LoadGameSession;

enum class LoadScenario
{
	Idle,
	Move,
	Combat,
	Mix,
};

class MetricsCollector
{
public:
	void AddLoginRtt(int32 ms);
	void AddEnterRtt(int32 ms);
	void AddMoveRtt(int32 ms);
	void AddSkillRtt(int32 ms);

	void IncConnectFail();
	void IncLoginFail();
	void IncEnterFail();
	void IncError();

	void IncLoginSuccess();
	void IncEnterSuccess();
	void IncMoveSend();
	void IncSkillSend();
	void IncHeartbeatSend();

	struct Snapshot
	{
		uint64 timestampMs = 0;
		int32 activeClients = 0;
		int32 inGameClients = 0;
		int32 connectFail = 0;
		int32 loginSuccess = 0;
		int32 loginFail = 0;
		int32 enterSuccess = 0;
		int32 enterFail = 0;
		int32 errors = 0;
		int32 moveSend = 0;
		int32 skillSend = 0;
		int32 heartbeatSend = 0;

		float loginAvg = 0.0f;
		float loginP95 = 0.0f;
		float enterAvg = 0.0f;
		float enterP95 = 0.0f;
		float moveAvg = 0.0f;
		float moveP95 = 0.0f;
		float skillAvg = 0.0f;
		float skillP95 = 0.0f;
	};

	Snapshot MakeSnapshotAndReset(uint64 nowMs, int32 activeClients, int32 inGameClients);

private:
	static void CalcAvgP95(const std::vector<int32>& samples, float& outAvg, float& outP95);

private:
	std::mutex _mtx;
	std::vector<int32> _loginRtts;
	std::vector<int32> _enterRtts;
	std::vector<int32> _moveRtts;
	std::vector<int32> _skillRtts;

	std::atomic<int32> _connectFail{ 0 };
	std::atomic<int32> _loginSuccess{ 0 };
	std::atomic<int32> _loginFail{ 0 };
	std::atomic<int32> _enterSuccess{ 0 };
	std::atomic<int32> _enterFail{ 0 };
	std::atomic<int32> _errors{ 0 };

	std::atomic<int32> _moveSend{ 0 };
	std::atomic<int32> _skillSend{ 0 };
	std::atomic<int32> _heartbeatSend{ 0 };
};

class LoadClient : public std::enable_shared_from_this<LoadClient>
{
    friend class LoadClientManager;
public:
	enum class State
	{
		Init,
		ConnectingLogin,
		LoginPending,
		LoginOk,
		ConnectingGame,
		EnterPending,
		InGame,
		Failed,
		Exit,
	};

	LoadClient(LoadClientManager* manager, int32 index, const std::string& userId, const std::string& password);

	void Start(uint64 nowMs);
	void Tick(uint64 nowMs);

	void OnLoginConnected();
	void OnLoginDisconnected();
	void OnGameConnected();
	void OnGameDisconnected();

	void OnLoginResponse(const Protocol::S_LOGIN& pkt, uint64 nowMs);
	void OnEnterGameResponse(const Protocol::S_ENTER_GAME& pkt, uint64 nowMs);
	void OnMoveAck(const Protocol::S_MOVE& pkt, uint64 nowMs);
	void OnSkillAck(const Protocol::S_SKILL& pkt, uint64 nowMs);

	State GetState() const { return _state.load(std::memory_order_acquire); }
	uint64 GetPlayerId() const { return _playerId.load(std::memory_order_acquire); }
	const std::string& GetUserId() const { return _userId; }

	void AttachLoginSession(const std::shared_ptr<LoadLoginSession>& session);
	void AttachGameSession(const std::shared_ptr<LoadGameSession>& session);

	bool IsAlive() const;
	bool IsInGame() const;

private:
	void SendLogin(uint64 nowMs);
	void SendEnterGame(uint64 nowMs);
	void SendMove(uint64 nowMs);
	void SendSkill(uint64 nowMs);
	void SendHeartbeat(uint64 nowMs);

	void UpdatePositionRandomLocked(float maxStep, uint64 nowMs);
	bool BuildPathLocked(uint64 nowMs);

private:
	LoadClientManager* _manager = nullptr;
	int32 _index = 0;
	std::string _userId;
	std::string _password;

	std::atomic<State> _state{ State::Init };

	std::mutex _mtx;
	std::string _token;
	std::atomic<uint64> _playerId{ 0 };
	Protocol::PositionInfo _pos;
	float _moveSpeed = 1.0f;
	uint32 _moveSeq = 0;
	Vector<Protocol::PositionInfo> _path;
	size_t _pathIndex = 0;
	uint64 _lastRepathMs = 0;

	uint64 _connectStartMs = 0;
	std::atomic<uint64> _loginStartMs{ 0 };
	std::atomic<uint64> _enterStartMs{ 0 };
	uint64 _lastMoveMs = 0;
	uint64 _lastSkillMs = 0;
	uint64 _lastHeartbeatMs = 0;
	std::atomic<uint64> _lastMoveSentMs{ 0 };
	std::atomic<uint64> _lastSkillSentMs{ 0 };

	std::shared_ptr<LoadLoginSession> _loginSession;
	std::shared_ptr<LoadGameSession> _gameSession;
};

class LoadLoginSession : public PacketSession
{
public:
	LoadLoginSession() {}
	virtual ~LoadLoginSession() {}

	virtual void OnConnected() override;
	virtual void OnDisconnected() override;
	virtual void OnRecvPacket(BYTE* buffer, int32 len) override;
	virtual void OnSend(int32 len) override {}

	void SetOwner(const std::weak_ptr<LoadClient>& owner) { _owner = owner; }
	std::shared_ptr<LoadClient> GetOwner() const { return _owner.lock(); }

private:
	std::weak_ptr<LoadClient> _owner;
};

class LoadGameSession : public PacketSession
{
public:
	LoadGameSession() {}
	virtual ~LoadGameSession() {}

	virtual void OnConnected() override;
	virtual void OnDisconnected() override;
	virtual void OnRecvPacket(BYTE* buffer, int32 len) override;
	virtual void OnSend(int32 len) override {}

	void SetOwner(const std::weak_ptr<LoadClient>& owner) { _owner = owner; }
	std::shared_ptr<LoadClient> GetOwner() const { return _owner.lock(); }

private:
	std::weak_ptr<LoadClient> _owner;
};

class LoadClientManager
{
    friend class LoadClient;
public:
	LoadClientManager();

	bool Init(const LoadClientConfig& config);
	void Start();
	void Stop();
	bool IsRunning() const { return _running.load(std::memory_order_acquire); }

	void Update(uint64 nowMs);

	const LoadClientConfig& GetConfig() const { return _config; }
	LoadScenario GetScenario() const { return _scenario; }
	MetricsCollector& Metrics() { return _metrics; }

	IocpCoreRef GetIocpCore() const { return _iocpCore; }
	ClientServiceRef GetLoginService() const { return _loginService; }
	ClientServiceRef GetGameService() const { return _gameService; }
	std::shared_ptr<NavSystem> GetNavSystem() const { return _navSystem; }
	bool HasNavSystem() const { return _navReady && _navSystem != nullptr; }

	void RequestGameConnect(const std::shared_ptr<LoadClient>& client, uint64 nowMs);

	void OnLoginPacket(const std::shared_ptr<LoadClient>& client, const Protocol::S_LOGIN& pkt, uint64 nowMs);
	void OnEnterPacket(const std::shared_ptr<LoadClient>& client, const Protocol::S_ENTER_GAME& pkt, uint64 nowMs);
	void OnMovePacket(const std::shared_ptr<LoadClient>& client, const Protocol::S_MOVE& pkt, uint64 nowMs);
	void OnSkillPacket(const std::shared_ptr<LoadClient>& client, const Protocol::S_SKILL& pkt, uint64 nowMs);

private:
	std::shared_ptr<LoadClient> CreateClient();
	std::shared_ptr<LoadLoginSession> CreateLoginSession(const std::shared_ptr<LoadClient>& client);
	std::shared_ptr<LoadGameSession> CreateGameSession(const std::shared_ptr<LoadClient>& client);

	void RampUpIfNeeded(uint64 nowMs);
	void LogIfNeeded(uint64 nowMs);
	bool InitNavMesh();

	std::string BuildAccountName(int32 index) const;

private:
	LoadClientConfig _config;
	LoadScenario _scenario = LoadScenario::Idle;

	std::atomic<bool> _running{ false };
	std::atomic<bool> _stopping{ false };

	IocpCoreRef _iocpCore;
	ClientServiceRef _loginService;
	ClientServiceRef _gameService;
	std::shared_ptr<NavSystem> _navSystem;
	bool _navReady = false;

    uint64 _startMs = 0;
    uint64 _holdStartMs = 0;
    uint64 _nextRampMs = 0;
    uint64 _nextLogMs = 0;
	int32 _desiredCcu = 0;

	int32 _nextAccountIndex = 0;
	std::vector<std::shared_ptr<LoadClient>> _clients;

	MetricsCollector _metrics;

	std::mutex _fileMtx;
	std::ofstream* _csv = nullptr;
};
