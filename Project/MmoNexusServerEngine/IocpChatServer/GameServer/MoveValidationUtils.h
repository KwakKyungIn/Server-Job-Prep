#pragma once
#include "pch.h"
#include "Struct.pb.h"
#include <algorithm>
#include <cmath>

namespace MoveValidate
{
	// 시간차 계산 시 언더플로우 방지
	// uint32 특성상 오버플로우가 발생해도 뺄셈 연산은 정상적인 차이값을 보장함
	inline uint32 Delta32(uint32 cur, uint32 prev)
	{
		return static_cast<uint32>(cur - prev);
	}

	// 시퀀스 넘버 비교 (패킷 순서 보장용)
	// 단순히 크기 비교를 하면 오버플로우 시점에 문제가 생기므로
	// uint32 범위의 절반(0x80000000)을 기준으로 더 가까운 쪽을 판단함
	inline bool IsSeqNewer(uint32 seq, uint32 lastSeq)
	{
		if (seq == lastSeq) return false;
		return static_cast<uint32>(seq - lastSeq) < 0x80000000u;
	}

	// 클라이언트 타임스탬프 기반 Delta Time 계산
	// 랙 스위칭(Lag Switching)이나 패킷 조작으로 시간이 튀는 것을 막기 위해
	// 서버에서 허용하는 최소/최대 프레임 시간(dtMin, dtMax)으로 클램핑함
	inline float ComputeDtSec(uint32 clientTimeMs, uint32 lastClientTimeMs,
		float dtMin = 0.02f, float dtMax = 0.25f, bool hasStamp = true)
	{
		if (!hasStamp) return dtMax;

		const uint32 dtMs = Delta32(clientTimeMs, lastClientTimeMs);
		float dt = static_cast<float>(dtMs) * 0.001f;

		// 비정상적인 값(NaN, 음수 등)이 들어오면 강제로 최대 지연시간으로 맞춤
		if (!std::isfinite(dt) || dt < 0.f)
			dt = dtMax;

		dt = max(dtMin, min(dt, dtMax));
		return dt;
	}

	enum class SpeedPolicy : uint8 { OK = 0, CLAMPED = 1 };

	struct SpeedCheckResult
	{
		SpeedPolicy policy = SpeedPolicy::OK;
		float dtSec = 0.f;
		float reqDist2D = 0.f;
		float maxDist = 0.f;
	};

	// 2D 이동 검증 메인 로직 (Server Authority)
	// 클라이언트가 보낸 위치가 이동 속도 대비 이론적으로 가능한 범위인지 체크함
	// 네트워크 지연을 고려해 약간의 오차(tolerance)를 허용함
	inline SpeedCheckResult CheckSpeed2D(const Protocol::PositionInfo& cur,
		const Protocol::PositionInfo& req,
		float dtSec, float speed, float tolerance,
		Protocol::PositionInfo& outPos)
	{
		SpeedCheckResult r;
		r.dtSec = dtSec;

		// 이동하려는 거리 계산 (피타고라스)
		const float dx = req.x() - cur.x();
		const float dz = req.z() - cur.z();
		const float dist = std::sqrt(dx * dx + dz * dz);

		r.reqDist2D = dist;
		// 최대 이동 가능 거리 = 속도 * 시간 + 허용오차
		r.maxDist = speed * dtSec + tolerance;

		outPos = req;

		// 허용 범위 내라면 통과
		if (dist <= r.maxDist || dist <= 1e-6f)
		{
			r.policy = SpeedPolicy::OK;
			return r;
		}

		// 범위를 초과했다면 스피드핵이나 랙으로 간주하고 위치를 보정(Clamp)함
		// 이동 벡터의 방향은 유지하되, 거리를 최대 허용 거리로 제한
		const float inv = 1.0f / dist;
		const float nx = dx * inv;
		const float nz = dz * inv;

		outPos.set_x(cur.x() + nx * r.maxDist);
		outPos.set_z(cur.z() + nz * r.maxDist);

		// Y축(높이)은 NavMesh나 중력 로직이 별도로 있어서 여기선 검증 안 함
		outPos.set_y(req.y());

		// 회전이나 상태값은 클라이언트 요청을 존중 (시각적 요소)
		outPos.set_yaw(req.yaw());
		outPos.set_state(req.state());
		outPos.set_actionstate(req.actionstate());

		r.policy = SpeedPolicy::CLAMPED;
		return r;
	}
}