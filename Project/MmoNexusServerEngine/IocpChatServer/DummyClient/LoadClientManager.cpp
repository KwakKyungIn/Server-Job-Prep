#include "pch.h"
#include "LoadClientManager.h"
#include "ServerPacketHandler.h"
#include "../GameServer/NavSystem.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

static LoadScenario ParseScenarioType(const std::string& raw)
{
	std::string s = raw;
	std::transform(s.begin(), s.end(), s.begin(), ::tolower);
	if (s == "move") return LoadScenario::Move;
	if (s == "combat") return LoadScenario::Combat;
	if (s == "mix") return LoadScenario::Mix;
	if (s == "persistence") return LoadScenario::Persistence;
	return LoadScenario::Idle;
}

static std::wstring ToWString(const std::string& s)
{
	return std::wstring(s.begin(), s.end());
}

static bool IsAbsolutePath(const std::string& path)
{
	if (path.size() >= 2 && path[1] == ':')
		return true;
	if (path.size() >= 2 && path[0] == '\\' && path[1] == '\\')
		return true;
	if (!path.empty() && (path[0] == '/' || path[0] == '\\'))
		return true;
	return false;
}

static std::string GetDirName(const std::string& path)
{
	const size_t pos = path.find_last_of("/\\");
	if (pos == std::string::npos)
		return std::string();
	return path.substr(0, pos + 1);
}

static std::string JoinPath(const std::string& dir, const std::string& file)
{
	if (dir.empty())
		return file;
	const char back = dir.back();
	if (back == '/' || back == '\\')
		return dir + file;
	return dir + "\\" + file;
}

static bool LoadNavMeshPathFromMaps(const std::string& mapsPath, int32 mapId, std::string& outNavPath)
{
	std::ifstream ifs(mapsPath);
	if (!ifs.is_open())
		return false;

	json j;
	try
	{
		ifs >> j;
	}
	catch (const std::exception&)
	{
		return false;
	}

	if (!j.contains("maps") || !j["maps"].is_array())
		return false;

	for (const auto& m : j["maps"])
	{
		if (m.value("mapId", 0) != mapId)
			continue;

		outNavPath = m.value("navMeshPath", std::string());
		return !outNavPath.empty();
	}

	return false;
}

// ================================
// MetricsCollector
// ================================
void MetricsCollector::AddLoginRtt(int32 ms)
{
	std::lock_guard<std::mutex> lock(_mtx);
	_loginRtts.push_back(ms);
}

void MetricsCollector::AddEnterRtt(int32 ms)
{
	std::lock_guard<std::mutex> lock(_mtx);
	_enterRtts.push_back(ms);
}

void MetricsCollector::AddMoveRtt(int32 ms)
{
	std::lock_guard<std::mutex> lock(_mtx);
	_moveRtts.push_back(ms);
}

void MetricsCollector::AddSkillRtt(int32 ms)
{
	std::lock_guard<std::mutex> lock(_mtx);
	_skillRtts.push_back(ms);
}

void MetricsCollector::IncConnectFail() { _connectFail.fetch_add(1); }
void MetricsCollector::IncLoginFail() { _loginFail.fetch_add(1); }
void MetricsCollector::IncEnterFail() { _enterFail.fetch_add(1); }
void MetricsCollector::IncError() { _errors.fetch_add(1); }

void MetricsCollector::IncLoginSuccess() { _loginSuccess.fetch_add(1); }
void MetricsCollector::IncEnterSuccess() { _enterSuccess.fetch_add(1); }
void MetricsCollector::IncMoveSend() { _moveSend.fetch_add(1); }
void MetricsCollector::IncSkillSend() { _skillSend.fetch_add(1); }
void MetricsCollector::IncHeartbeatSend() { _heartbeatSend.fetch_add(1); }
void MetricsCollector::IncQuickSlotSend() { _quickslotSend.fetch_add(1); }

void MetricsCollector::CalcAvgP95(const std::vector<int32>& samples, float& outAvg, float& outP95)
{
	outAvg = 0.0f;
	outP95 = 0.0f;
	if (samples.empty())
		return;

	int64 sum = 0;
	for (int32 v : samples)
		sum += v;
	outAvg = static_cast<float>(sum) / static_cast<float>(samples.size());

	std::vector<int32> sorted = samples;
	std::sort(sorted.begin(), sorted.end());
	const size_t idx = static_cast<size_t>(std::ceil(sorted.size() * 0.95f)) - 1;
	outP95 = static_cast<float>(sorted[(std::min)(idx, sorted.size() - 1)]);
}

MetricsCollector::Snapshot MetricsCollector::MakeSnapshotAndReset(uint64 nowMs, int32 activeClients, int32 inGameClients)
{
	Snapshot snap;
	snap.timestampMs = nowMs;
	snap.activeClients = activeClients;
	snap.inGameClients = inGameClients;

	snap.connectFail = _connectFail.exchange(0);
	snap.loginSuccess = _loginSuccess.exchange(0);
	snap.loginFail = _loginFail.exchange(0);
	snap.enterSuccess = _enterSuccess.exchange(0);
	snap.enterFail = _enterFail.exchange(0);
	snap.errors = _errors.exchange(0);
	snap.moveSend = _moveSend.exchange(0);
	snap.skillSend = _skillSend.exchange(0);
	snap.heartbeatSend = _heartbeatSend.exchange(0);
	snap.quickslotSend = _quickslotSend.exchange(0);

	std::vector<int32> loginSamples;
	std::vector<int32> enterSamples;
	std::vector<int32> moveSamples;
	std::vector<int32> skillSamples;

	{
		std::lock_guard<std::mutex> lock(_mtx);
		loginSamples.swap(_loginRtts);
		enterSamples.swap(_enterRtts);
		moveSamples.swap(_moveRtts);
		skillSamples.swap(_skillRtts);
	}

	CalcAvgP95(loginSamples, snap.loginAvg, snap.loginP95);
	CalcAvgP95(enterSamples, snap.enterAvg, snap.enterP95);
	CalcAvgP95(moveSamples, snap.moveAvg, snap.moveP95);
	CalcAvgP95(skillSamples, snap.skillAvg, snap.skillP95);

	return snap;
}

// ================================
// LoadClient
// ================================
LoadClient::LoadClient(LoadClientManager* manager, int32 index, const std::string& userId, const std::string& password)
	: _manager(manager), _index(index), _userId(userId), _password(password)
{
	_pos.set_x(0.0f);
	_pos.set_y(0.0f);
	_pos.set_z(0.0f);
	_pos.set_yaw(0.0f);
	_pos.set_state(Protocol::MOVE_IDLE);
	_pos.set_actionstate(Protocol::ACTION_IDLE);
}

void LoadClient::Start(uint64 nowMs)
{
	_state.store(State::ConnectingLogin, std::memory_order_release);
	_connectStartMs = nowMs;
	_loginStartMs.store(0, std::memory_order_release);
	_enterStartMs.store(0, std::memory_order_release);

	if (_manager)
		_manager->CreateLoginSession(shared_from_this());
}

void LoadClient::AttachLoginSession(const std::shared_ptr<LoadLoginSession>& session)
{
	_loginSession = session;
}

void LoadClient::AttachGameSession(const std::shared_ptr<LoadGameSession>& session)
{
	_gameSession = session;
}

bool LoadClient::IsAlive() const
{
	State st = _state.load(std::memory_order_acquire);
	return (st != State::Failed && st != State::Exit);
}

bool LoadClient::IsInGame() const
{
	return _state.load(std::memory_order_acquire) == State::InGame;
}

void LoadClient::OnLoginConnected()
{
	SendLogin(::GetTickCount64());
}

void LoadClient::OnLoginDisconnected()
{
	State st = _state.load(std::memory_order_acquire);
	if (st == State::LoginOk || st == State::ConnectingGame || st == State::EnterPending || st == State::InGame)
		return; // intentional close after login OK or not needed

	_state.store(State::Failed, std::memory_order_release);
	if (_manager) _manager->Metrics().IncError();
}

void LoadClient::OnGameConnected()
{
	SendEnterGame(::GetTickCount64());
}

void LoadClient::OnGameDisconnected()
{
	_state.store(State::Failed, std::memory_order_release);
	if (_manager) _manager->Metrics().IncError();
}

void LoadClient::OnLoginResponse(const Protocol::S_LOGIN& pkt, uint64 nowMs)
{
	if (!pkt.success())
	{
		_state.store(State::Failed, std::memory_order_release);
		if (_manager) _manager->Metrics().IncLoginFail();
		return;
	}

	{
		std::lock_guard<std::mutex> lock(_mtx);
		_token = pkt.token();
	}

	if (_manager)
	{
		_manager->Metrics().IncLoginSuccess();
		const uint64 loginStart = _loginStartMs.load(std::memory_order_acquire);
		if (loginStart > 0)
			_manager->Metrics().AddLoginRtt(static_cast<int32>(nowMs - loginStart));
	}

	_state.store(State::LoginOk, std::memory_order_release);

	if (_manager && _manager->GetConfig().options.keepLoginConnection == false)
	{
		if (_loginSession && _loginSession->IsConnected())
			_loginSession->Disconnect(L"Login Done");
	}
}

void LoadClient::OnEnterGameResponse(const Protocol::S_ENTER_GAME& pkt, uint64 nowMs)
{
	if (!pkt.success())
	{
		_state.store(State::Failed, std::memory_order_release);
		if (_manager) _manager->Metrics().IncEnterFail();
		return;
	}

	{
		std::lock_guard<std::mutex> lock(_mtx);
		_playerId.store(pkt.myplayer().playerid(), std::memory_order_release);
		_pos = pkt.myplayer().posinfo();
		_path.clear();
		_pathIndex = 0;
		_lastRepathMs = 0;
		_lastAckX = _pos.x();
		_lastAckZ = _pos.z();
		_stuckAckCount = 0;
		_forceRepath = false;
		_lastQuickSlotMs = 0;
		_quickSlotCursor = 0;
		_quickSlotSkillCursor = 1;
		_quickSlotEventCount = 0;
		if (pkt.has_myplayer() && pkt.myplayer().has_statinfo())
		{
			const int32 spd = pkt.myplayer().statinfo().speed();
			if (spd > 0)
				_moveSpeed = static_cast<float>(spd);
		}
	}

	if (_manager)
	{
		_manager->Metrics().IncEnterSuccess();
		const uint64 enterStart = _enterStartMs.load(std::memory_order_acquire);
		if (enterStart > 0)
			_manager->Metrics().AddEnterRtt(static_cast<int32>(nowMs - enterStart));
	}

	_state.store(State::InGame, std::memory_order_release);
}

void LoadClient::OnMoveAck(const Protocol::S_MOVE& pkt, uint64 nowMs)
{
	if (pkt.objectid() != _playerId.load(std::memory_order_acquire))
		return;

	{
		std::lock_guard<std::mutex> lock(_mtx);
		if (pkt.has_posinfo())
		{
			const Protocol::PositionInfo& ackPos = pkt.posinfo();
			const float dx = ackPos.x() - _lastAckX;
			const float dz = ackPos.z() - _lastAckZ;
			const float moved = std::sqrt(dx * dx + dz * dz);

			const bool pathActive = !_path.empty() && _pathIndex < _path.size();
			if (pathActive && moved < 0.03f)
			{
				if (_stuckAckCount < 1000000)
					++_stuckAckCount;

				if (_stuckAckCount >= 3)
				{
					// The server keeps snapping us to the same point (e.g. wall hit),
					// so drop the current path and force a fresh random goal.
					_path.clear();
					_pathIndex = 0;
					_lastRepathMs = 0;
					_forceRepath = true;
					_stuckAckCount = 0;
				}
			}
			else
			{
				_stuckAckCount = 0;
			}

			_pos = ackPos;
			_lastAckX = ackPos.x();
			_lastAckZ = ackPos.z();
		}
	}

	const uint64 lastMove = _lastMoveSentMs.load(std::memory_order_acquire);
	if (lastMove > 0 && _manager)
		_manager->Metrics().AddMoveRtt(static_cast<int32>(nowMs - lastMove));
}

void LoadClient::OnSkillAck(const Protocol::S_SKILL& pkt, uint64 nowMs)
{
	if (pkt.objectid() != _playerId.load(std::memory_order_acquire))
		return;

	const uint64 lastSkill = _lastSkillSentMs.load(std::memory_order_acquire);
	if (lastSkill > 0 && _manager)
		_manager->Metrics().AddSkillRtt(static_cast<int32>(nowMs - lastSkill));
}

void LoadClient::Tick(uint64 nowMs)
{
	const LoadClientConfig& cfg = _manager->GetConfig();
	State st = _state.load(std::memory_order_acquire);

	switch (st)
	{
	case State::ConnectingLogin:
		if (_connectStartMs > 0 && nowMs - _connectStartMs > static_cast<uint64>(cfg.timeouts.connectMs))
		{
			_state.store(State::Failed, std::memory_order_release);
			_manager->Metrics().IncConnectFail();
		}
		break;
	case State::LoginPending:
	{
		const uint64 loginStartMs = _loginStartMs.load(std::memory_order_acquire);
		if (loginStartMs > 0 && nowMs - loginStartMs > static_cast<uint64>(cfg.timeouts.loginMs))
		{
			_state.store(State::Failed, std::memory_order_release);
			_manager->Metrics().IncLoginFail();
		}
		break;
	}
	case State::LoginOk:
		_manager->RequestGameConnect(shared_from_this(), nowMs);
		break;
	case State::ConnectingGame:
		if (_connectStartMs > 0 && nowMs - _connectStartMs > static_cast<uint64>(cfg.timeouts.connectMs))
		{
			_state.store(State::Failed, std::memory_order_release);
			_manager->Metrics().IncConnectFail();
		}
		break;
	case State::EnterPending:
	{
		const uint64 enterStartMs = _enterStartMs.load(std::memory_order_acquire);
		if (enterStartMs > 0 && nowMs - enterStartMs > static_cast<uint64>(cfg.timeouts.enterMs))
		{
			_state.store(State::Failed, std::memory_order_release);
			_manager->Metrics().IncEnterFail();
		}
		break;
	}
	case State::InGame:
		SendHeartbeat(nowMs);

		if (_manager->GetScenario() == LoadScenario::Move || _manager->GetScenario() == LoadScenario::Mix)
			SendMove(nowMs);
		if (_manager->GetScenario() == LoadScenario::Combat || _manager->GetScenario() == LoadScenario::Mix)
			SendSkill(nowMs);
		if (_manager->GetScenario() == LoadScenario::Persistence)
			SendQuickSlot(nowMs);
		break;
	default:
		break;
	}
}

void LoadClient::SendLogin(uint64 nowMs)
{
	if (!_loginSession || !_loginSession->IsConnected())
	{
		_state.store(State::Failed, std::memory_order_release);
		if (_manager) _manager->Metrics().IncConnectFail();
		return;
	}

	Protocol::C_LOGIN pkt;
	pkt.set_userid(_userId);
	pkt.set_password(_password);

	_loginStartMs.store(nowMs, std::memory_order_release);
	_state.store(State::LoginPending, std::memory_order_release);

	auto sb = ServerPacketHandler::MakeSendBuffer(pkt);
	_loginSession->Send(sb);
}

void LoadClient::SendEnterGame(uint64 nowMs)
{
	if (!_gameSession || !_gameSession->IsConnected())
	{
		_state.store(State::Failed, std::memory_order_release);
		if (_manager) _manager->Metrics().IncConnectFail();
		return;
	}

	std::string token;
	{
		std::lock_guard<std::mutex> lock(_mtx);
		token = _token;
	}

	if (token.empty())
	{
		_state.store(State::Failed, std::memory_order_release);
		if (_manager) _manager->Metrics().IncEnterFail();
		return;
	}

	Protocol::C_ENTER_GAME pkt;
	pkt.set_token(token);
	pkt.set_channelid(_manager->GetConfig().channelId);
	pkt.set_mapid(_manager->GetConfig().mapId);

	_enterStartMs.store(nowMs, std::memory_order_release);
	_state.store(State::EnterPending, std::memory_order_release);

	auto sb = ServerPacketHandler::MakeSendBuffer(pkt);
	_gameSession->Send(sb);
}

void LoadClient::SendMove(uint64 nowMs)
{
	const LoadClientConfig& cfg = _manager->GetConfig();
	if (cfg.moveHz <= 0.0f)
		return;
	if (!_gameSession || !_gameSession->IsConnected())
		return;

	const uint64 interval = static_cast<uint64>(1000.0f / cfg.moveHz);
	if (_lastMoveMs != 0 && nowMs - _lastMoveMs < interval)
		return;

	Protocol::C_MOVE pkt;
	Protocol::PositionInfo posCopy;
	{
		std::lock_guard<std::mutex> lock(_mtx);
		float dtSec = 0.25f;
		if (_lastMoveMs != 0)
		{
			const float raw = static_cast<float>(nowMs - _lastMoveMs) * 0.001f;
			if (std::isfinite(raw) && raw >= 0.0f)
				dtSec = raw;
		}

		// Mirror server-side clamp window to avoid SPEED_EXCEEDED_CLAMP.
		dtSec = (std::max)(0.02f, (std::min)(dtSec, 0.25f));

		const float maxDist = _moveSpeed * dtSec + 0.30f;
		const float safeStep = (maxDist > 0.0f) ? (maxDist * 0.75f) : 0.0f;
		UpdatePositionRandomLocked(safeStep, nowMs);
		posCopy = _pos;
	}
	*pkt.mutable_posinfo() = posCopy;
	pkt.set_move_seq(++_moveSeq);
	pkt.set_client_time_ms(static_cast<uint32>(nowMs & 0xFFFFFFFF));

	auto sb = ServerPacketHandler::MakeSendBuffer(pkt);
	if (_gameSession)
		_gameSession->Send(sb);

	_lastMoveMs = nowMs;
	_lastMoveSentMs.store(nowMs, std::memory_order_release);
	if (_manager) _manager->Metrics().IncMoveSend();
}

void LoadClient::SendSkill(uint64 nowMs)
{
	const LoadClientConfig& cfg = _manager->GetConfig();
	if (cfg.skillHz <= 0.0f)
		return;
	if (!_gameSession || !_gameSession->IsConnected())
		return;

	const uint64 interval = static_cast<uint64>(1000.0f / cfg.skillHz);
	if (_lastSkillMs != 0 && nowMs - _lastSkillMs < interval)
		return;

	Protocol::C_SKILL pkt;
	pkt.set_skillid(1);
	{
		std::lock_guard<std::mutex> lock(_mtx);
		pkt.set_castyaw(_pos.yaw());
	}
	pkt.set_client_time_ms(static_cast<uint32>(nowMs & 0xFFFFFFFF));

	auto sb = ServerPacketHandler::MakeSendBuffer(pkt);
	if (_gameSession)
		_gameSession->Send(sb);

	_lastSkillMs = nowMs;
	_lastSkillSentMs.store(nowMs, std::memory_order_release);
	if (_manager) _manager->Metrics().IncSkillSend();
}

void LoadClient::SendHeartbeat(uint64 nowMs)
{
	const LoadClientConfig& cfg = _manager->GetConfig();
	if (cfg.heartbeatHz <= 0.0f)
		return;
	if (!_gameSession || !_gameSession->IsConnected())
		return;

	const uint64 interval = static_cast<uint64>(1000.0f / cfg.heartbeatHz);
	if (_lastHeartbeatMs != 0 && nowMs - _lastHeartbeatMs < interval)
		return;

	Protocol::C_HEART_BEAT_REQ pkt;
	auto sb = ServerPacketHandler::MakeSendBuffer(pkt);
	if (_gameSession)
		_gameSession->Send(sb);

	_lastHeartbeatMs = nowMs;
	if (_manager) _manager->Metrics().IncHeartbeatSend();
}

void LoadClient::SendQuickSlot(uint64 nowMs)
{
	const LoadClientConfig& cfg = _manager->GetConfig();
	if (cfg.quickslotHz <= 0.0f)
		return;
	if (!_gameSession || !_gameSession->IsConnected())
		return;

	const uint64 interval = static_cast<uint64>(1000.0f / cfg.quickslotHz);
	if (_lastQuickSlotMs != 0 && nowMs - _lastQuickSlotMs < interval)
		return;

	Protocol::C_SET_QUICKSLOT pkt;
	pkt.set_slotindex(_quickSlotCursor);

	if (((_quickSlotEventCount + 1) % 4) == 0)
	{
		pkt.set_reftype(Protocol::QS_NONE);
		pkt.set_refid(0);
	}
	else
	{
		pkt.set_reftype(Protocol::QS_SKILL);
		pkt.set_refid(static_cast<uint64>(_quickSlotSkillCursor));

		_quickSlotSkillCursor++;
		if (_quickSlotSkillCursor > 3)
			_quickSlotSkillCursor = 1;
	}

	auto sb = ServerPacketHandler::MakeSendBuffer(pkt);
	_gameSession->Send(sb);

	_lastQuickSlotMs = nowMs;
	_quickSlotEventCount++;
	_quickSlotCursor = (_quickSlotCursor + 1) % 4;

	if (_manager)
		_manager->Metrics().IncQuickSlotSend();
}

bool LoadClient::BuildPathLocked(uint64 nowMs)
{
	if (!_manager)
		return false;

	auto nav = _manager->GetNavSystem();
	if (!nav)
		return false;

	const LoadClientConfig& cfg = _manager->GetConfig();
	const float radius = (cfg.navMesh.goalRadius > 0.0f) ? cfg.navMesh.goalRadius : cfg.spawnCluster.radius;
	if (radius <= 0.0f)
		return false;

	const int attempts = (std::max)(1, cfg.navMesh.maxGoalAttempts);
	const float cx = cfg.spawnCluster.centerX;
	const float cy = cfg.spawnCluster.centerY;
	const float cz = cfg.spawnCluster.centerZ;

	thread_local std::mt19937 rng{ std::random_device{}() };
	std::uniform_real_distribution<float> angleDist(0.0f, 6.283185f);
	std::uniform_real_distribution<float> radiusDist(0.0f, 1.0f);

	for (int i = 0; i < attempts; ++i)
	{
		const float r = std::sqrt(radiusDist(rng)) * radius;
		const float ang = angleDist(rng);

		const float gx = cx + std::cos(ang) * r;
		const float gz = cz + std::sin(ang) * r;

		float snapY = cy;
		if (!nav->ResolvePoint(gx, cy, gz, snapY))
			continue;

		Protocol::PositionInfo start = _pos;
		Protocol::PositionInfo end;
		end.set_x(gx);
		end.set_y(snapY);
		end.set_z(gz);

		Vector<Vector3> waypoints;
		if (!nav->FindPathWaypoints(start, end, waypoints) || waypoints.empty())
			continue;

		_path.clear();
		_path.reserve(waypoints.size());
		for (const auto& wp : waypoints)
		{
			Protocol::PositionInfo p;
			p.set_x(wp.x);
			p.set_y(wp.y);
			p.set_z(wp.z);
			_path.push_back(p);
		}

		_pathIndex = 0;
		_lastRepathMs = nowMs;
		return true;
	}

	return false;
}

void LoadClient::UpdatePositionRandomLocked(float maxStep, uint64 nowMs)
{
	const LoadClientConfig& cfg = _manager->GetConfig();
	if (maxStep <= 0.0f)
		return;

	if (_manager && _manager->HasNavSystem())
	{
		const int repathSec = cfg.navMesh.repathIntervalSec;
		if (_forceRepath || _path.empty() || _pathIndex >= _path.size() ||
			(repathSec > 0 && nowMs - _lastRepathMs >= static_cast<uint64>(repathSec) * 1000))
		{
			_forceRepath = false;
			if (!BuildPathLocked(nowMs))
				return;
		}

		if (_pathIndex >= _path.size())
			return;

		const Protocol::PositionInfo& target = _path[_pathIndex];
		const float dx = target.x() - _pos.x();
		const float dz = target.z() - _pos.z();
		const float dist = std::sqrt(dx * dx + dz * dz);

		const float reach = (cfg.navMesh.waypointReachDist > 0.0f) ? cfg.navMesh.waypointReachDist : 0.5f;
		if (dist <= reach)
		{
			_pathIndex++;
			return;
		}

		const float step = (std::min)(maxStep, dist);
		const float inv = (dist > 1e-6f) ? (1.0f / dist) : 0.0f;
		float nx = _pos.x() + dx * inv * step;
		float nz = _pos.z() + dz * inv * step;
		float ny = _pos.y();

		if (auto nav = _manager->GetNavSystem())
		{
			float snapY = ny;
			if (nav->ResolvePoint(nx, ny, nz, snapY))
				ny = snapY;
		}

		_pos.set_x(nx);
		_pos.set_y(ny);
		_pos.set_z(nz);
		_pos.set_yaw(std::atan2(dx, dz) * 180.0f / 3.141592f);
		_pos.set_state(Protocol::MOVE_RUN);
		_pos.set_actionstate(Protocol::ACTION_IDLE);

		if (dist <= step + reach)
			_pathIndex++;

		return;
	}

	const float radius = cfg.spawnCluster.radius;
	if (radius <= 0.0f)
		return;

	thread_local std::mt19937 rng{ std::random_device{}() };
	std::uniform_real_distribution<float> angleDist(0.0f, 6.283185f);
	std::uniform_real_distribution<float> stepDist(0.0f, maxStep);

	const float angle = angleDist(rng);
	const float step = stepDist(rng);

	const float dx = std::cos(angle) * step;
	const float dz = std::sin(angle) * step;

	float nx = _pos.x() + dx;
	float nz = _pos.z() + dz;

	const float cx = cfg.spawnCluster.centerX;
	const float cz = cfg.spawnCluster.centerZ;
	const float distX = nx - cx;
	const float distZ = nz - cz;
	const float distSqr = distX * distX + distZ * distZ;

	if (distSqr > radius * radius)
	{
		const float dist = std::sqrt(distSqr);
		const float targetX = cx + distX * (radius / dist);
		const float targetZ = cz + distZ * (radius / dist);

		const float tdx = targetX - _pos.x();
		const float tdz = targetZ - _pos.z();
		const float tdist = std::sqrt(tdx * tdx + tdz * tdz);

		if (tdist > maxStep && tdist > 1e-6f)
		{
			const float inv = 1.0f / tdist;
			nx = _pos.x() + tdx * inv * maxStep;
			nz = _pos.z() + tdz * inv * maxStep;
		}
		else
		{
			nx = targetX;
			nz = targetZ;
		}
	}

	_pos.set_x(nx);
	_pos.set_y(cfg.spawnCluster.centerY);
	_pos.set_z(nz);
	_pos.set_yaw(angle * 180.0f / 3.141592f);
	_pos.set_state(Protocol::MOVE_RUN);
	_pos.set_actionstate(Protocol::ACTION_IDLE);
}

// ================================
// LoadLoginSession / LoadGameSession
// ================================
void LoadLoginSession::OnConnected()
{
	if (auto owner = _owner.lock())
		owner->OnLoginConnected();
}

void LoadLoginSession::OnDisconnected()
{
	if (auto owner = _owner.lock())
		owner->OnLoginDisconnected();
}

void LoadLoginSession::OnRecvPacket(BYTE* buffer, int32 len)
{
	PacketSessionRef session = GetPacketSessionRef();
	ServerPacketHandler::HandlePacket(session, buffer, len);
}

void LoadGameSession::OnConnected()
{
	if (auto owner = _owner.lock())
		owner->OnGameConnected();
}

void LoadGameSession::OnDisconnected()
{
	if (auto owner = _owner.lock())
		owner->OnGameDisconnected();
}

void LoadGameSession::OnRecvPacket(BYTE* buffer, int32 len)
{
	PacketSessionRef session = GetPacketSessionRef();
	ServerPacketHandler::HandlePacket(session, buffer, len);
}

// ================================
// LoadClientManager
// ================================
LoadClientManager::LoadClientManager()
{
}

bool LoadClientManager::Init(const LoadClientConfig& config)
{
	_config = config;
	_scenario = ParseScenarioType(config.scenario);
	_nextAccountIndex = config.account.start;
	_desiredCcu = 0;

	_iocpCore = MakeShared<IocpCore>();
	if (_iocpCore == nullptr)
		return false;

	_loginService = MakeShared<ClientService>(
		NetAddress(ToWString(_config.loginServer.ip), _config.loginServer.port),
		_iocpCore,
		[]() { return MakeShared<LoadLoginSession>(); },
		0);

	_gameService = MakeShared<ClientService>(
		NetAddress(ToWString(_config.gameServer.ip), _config.gameServer.port),
		_iocpCore,
		[]() { return MakeShared<LoadGameSession>(); },
		0);

	if (!_loginService || !_gameService)
		return false;

	InitNavMesh();
	return true;
}

bool LoadClientManager::InitNavMesh()
{
	if (_config.navMesh.enabled == false)
		return false;

	std::string navPath = _config.navMesh.navMeshPath;
	if (navPath.empty())
	{
		const std::string mapsPath = _config.navMesh.mapsPath.empty() ? "Maps.json" : _config.navMesh.mapsPath;
		if (!LoadNavMeshPathFromMaps(mapsPath, _config.mapId, navPath))
		{
			std::cout << " [LoadClient] Maps.json load failed or navMeshPath missing (mapId=" << _config.mapId << ")\n";
			return false;
		}

		if (!IsAbsolutePath(navPath))
			navPath = JoinPath(GetDirName(mapsPath), navPath);
	}

	if (navPath.empty())
		return false;

	_navSystem = std::make_shared<NavSystem>();
	if (!_navSystem->Load(navPath))
	{
		std::cout << " [LoadClient] NavMesh load failed: " << navPath << "\n";
		_navSystem.reset();
		return false;
	}

	_navReady = true;
	std::cout << " [LoadClient] NavMesh loaded: " << navPath << "\n";
	return true;
}

void LoadClientManager::Start()
{
	_running.store(true, std::memory_order_release);
	_stopping.store(false, std::memory_order_release);

	_startMs = ::GetTickCount64();
	_holdStartMs = 0;
	_nextRampMs = _startMs;
	_nextLogMs = _startMs + static_cast<uint64>(_config.options.logIntervalSec) * 1000;

	if (_config.options.csvOutput)
	{
		std::lock_guard<std::mutex> lock(_fileMtx);
		_csv = new std::ofstream(_config.options.csvPath, std::ios::out | std::ios::trunc);
		if (_csv && _csv->is_open())
		{
			(*_csv) << "timestamp_ms,active,ingame,connect_fail,login_success,login_fail,enter_success,enter_fail,errors,move_send,skill_send,heartbeat_send,quickslot_send,login_avg,login_p95,enter_avg,enter_p95,move_avg,move_p95,skill_avg,skill_p95\n";
		}
		else
		{
			delete _csv;
			_csv = nullptr;
		}
	}
}

void LoadClientManager::Stop()
{
	_stopping.store(true, std::memory_order_release);
	_running.store(false, std::memory_order_release);

	for (auto& c : _clients)
	{
		if (!c) continue;
		// Session shutdown handled by CloseService.
	}

	if (_loginService)
		_loginService->CloseService();
	if (_gameService)
		_gameService->CloseService();

	{
		std::lock_guard<std::mutex> lock(_fileMtx);
		if (_csv)
		{
			_csv->close();
			delete _csv;
			_csv = nullptr;
		}
	}
}

void LoadClientManager::Update(uint64 nowMs)
{
	if (!IsRunning())
		return;

	RampUpIfNeeded(nowMs);

	for (auto& c : _clients)
	{
		if (!c) continue;
		c->Tick(nowMs);
	}

	LogIfNeeded(nowMs);

    if (_desiredCcu >= _config.ccuTarget && _config.holdSec > 0)
    {
        if (_holdStartMs == 0)
            _holdStartMs = nowMs;
        else if (nowMs - _holdStartMs >= static_cast<uint64>(_config.holdSec) * 1000)
            Stop();
    }
}

void LoadClientManager::RampUpIfNeeded(uint64 nowMs)
{
	if (nowMs < _nextRampMs)
		return;

	if (_desiredCcu < _config.ccuTarget)
	{
		_desiredCcu = (std::min)(_config.ccuTarget, _desiredCcu + _config.rampStep);
		_nextRampMs = nowMs + static_cast<uint64>(_config.rampIntervalSec) * 1000;
	}

    int32 activeCount = 0;
    for (const auto& c : _clients)
    {
        if (!c) continue;
        if (_config.options.replaceOnFail)
        {
            if (c->IsAlive()) activeCount++;
        }
        else
        {
            activeCount++;
        }
    }

	while (activeCount < _desiredCcu)
	{
		auto client = CreateClient();
		if (!client)
			break;
		client->Start(nowMs);
		activeCount++;
	}
}

void LoadClientManager::LogIfNeeded(uint64 nowMs)
{
	if (_config.options.logIntervalSec <= 0)
		return;
	if (nowMs < _nextLogMs)
		return;

	int32 activeCount = 0;
	int32 inGameCount = 0;
	for (const auto& c : _clients)
	{
		if (!c) continue;
		if (c->IsAlive()) activeCount++;
		if (c->IsInGame()) inGameCount++;
	}

	const auto snap = _metrics.MakeSnapshotAndReset(nowMs, activeCount, inGameCount);

	std::cout << "[LoadTest] active=" << snap.activeClients
		<< " ingame=" << snap.inGameClients
		<< " login(avg/p95)=" << snap.loginAvg << "/" << snap.loginP95
		<< " enter(avg/p95)=" << snap.enterAvg << "/" << snap.enterP95
		<< " move(avg/p95)=" << snap.moveAvg << "/" << snap.moveP95
		<< " skill(avg/p95)=" << snap.skillAvg << "/" << snap.skillP95
		<< " quickslot_send=" << snap.quickslotSend
		<< " errors=" << snap.errors
		<< std::endl;

	if (_csv)
	{
		std::lock_guard<std::mutex> lock(_fileMtx);
		if (_csv && _csv->is_open())
		{
			(*_csv) << snap.timestampMs << ','
				<< snap.activeClients << ','
				<< snap.inGameClients << ','
				<< snap.connectFail << ','
				<< snap.loginSuccess << ','
				<< snap.loginFail << ','
				<< snap.enterSuccess << ','
				<< snap.enterFail << ','
				<< snap.errors << ','
				<< snap.moveSend << ','
				<< snap.skillSend << ','
				<< snap.heartbeatSend << ','
				<< snap.quickslotSend << ','
				<< snap.loginAvg << ','
				<< snap.loginP95 << ','
				<< snap.enterAvg << ','
				<< snap.enterP95 << ','
				<< snap.moveAvg << ','
				<< snap.moveP95 << ','
				<< snap.skillAvg << ','
				<< snap.skillP95 << '\n';
		}
	}

	_nextLogMs = nowMs + static_cast<uint64>(_config.options.logIntervalSec) * 1000;
}

std::shared_ptr<LoadClient> LoadClientManager::CreateClient()
{
	const int32 maxIndex = _config.account.start + _config.account.count;
	if (_nextAccountIndex >= maxIndex)
		return nullptr;

	std::string name = BuildAccountName(_nextAccountIndex);
	_nextAccountIndex++;

	auto client = std::make_shared<LoadClient>(this, static_cast<int32>(_clients.size()), name, _config.account.password);
	_clients.push_back(client);
	return client;
}

std::shared_ptr<LoadLoginSession> LoadClientManager::CreateLoginSession(const std::shared_ptr<LoadClient>& client)
{
	if (!_loginService)
		return nullptr;

	auto session = std::static_pointer_cast<LoadLoginSession>(_loginService->CreateSession());
	if (!session)
	{
		_metrics.IncConnectFail();
		return nullptr;
	}
	session->SetOwner(client);
	client->AttachLoginSession(session);

	if (session->Connect() == false)
	{
		_metrics.IncConnectFail();
	}
	return session;
}

std::shared_ptr<LoadGameSession> LoadClientManager::CreateGameSession(const std::shared_ptr<LoadClient>& client)
{
	if (!_gameService)
		return nullptr;

	auto session = std::static_pointer_cast<LoadGameSession>(_gameService->CreateSession());
	if (!session)
	{
		_metrics.IncConnectFail();
		return nullptr;
	}
	session->SetOwner(client);
	client->AttachGameSession(session);

	if (session->Connect() == false)
	{
		_metrics.IncConnectFail();
	}
	return session;
}

void LoadClientManager::RequestGameConnect(const std::shared_ptr<LoadClient>& client, uint64 nowMs)
{
	if (!client) return;

	if (client->GetState() != LoadClient::State::LoginOk)
		return;

	client->_state.store(LoadClient::State::ConnectingGame, std::memory_order_release);
	client->_connectStartMs = nowMs;
	CreateGameSession(client);
}

void LoadClientManager::OnLoginPacket(const std::shared_ptr<LoadClient>& client, const Protocol::S_LOGIN& pkt, uint64 nowMs)
{
	if (client)
		client->OnLoginResponse(pkt, nowMs);
}

void LoadClientManager::OnEnterPacket(const std::shared_ptr<LoadClient>& client, const Protocol::S_ENTER_GAME& pkt, uint64 nowMs)
{
	if (client)
		client->OnEnterGameResponse(pkt, nowMs);
}

void LoadClientManager::OnMovePacket(const std::shared_ptr<LoadClient>& client, const Protocol::S_MOVE& pkt, uint64 nowMs)
{
	if (client)
		client->OnMoveAck(pkt, nowMs);
}

void LoadClientManager::OnSkillPacket(const std::shared_ptr<LoadClient>& client, const Protocol::S_SKILL& pkt, uint64 nowMs)
{
	if (client)
		client->OnSkillAck(pkt, nowMs);
}

std::string LoadClientManager::BuildAccountName(int32 index) const
{
	std::ostringstream oss;
	oss << _config.account.prefix;
	oss << std::setfill('0') << std::setw(_config.account.padWidth) << index;
	return oss.str();
}
