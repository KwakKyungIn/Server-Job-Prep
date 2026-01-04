#pragma once
#include "pch.h"
#include "Struct.pb.h"
#include <algorithm>
#include <cmath>

namespace MoveValidate
{
	inline uint32 Delta32(uint32 cur, uint32 prev)
	{
		return static_cast<uint32>(cur - prev);
	}

	// wrap-safe newer compare (half range)
	inline bool IsSeqNewer(uint32 seq, uint32 lastSeq)
	{
		if (seq == lastSeq) return false;
		return static_cast<uint32>(seq - lastSeq) < 0x80000000u;
	}

	inline float ComputeDtSec(uint32 clientTimeMs, uint32 lastClientTimeMs,
		float dtMin = 0.02f, float dtMax = 0.25f, bool hasStamp = true)
	{
		if (!hasStamp) return dtMax;

		const uint32 dtMs = Delta32(clientTimeMs, lastClientTimeMs);
		float dt = static_cast<float>(dtMs) * 0.001f;

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

	inline SpeedCheckResult CheckSpeed2D(const Protocol::PositionInfo& cur,
		const Protocol::PositionInfo& req,
		float dtSec, float speed, float tolerance,
		Protocol::PositionInfo& outPos)
	{
		SpeedCheckResult r;
		r.dtSec = dtSec;

		const float dx = req.x() - cur.x();
		const float dz = req.z() - cur.z();
		const float dist = std::sqrt(dx * dx + dz * dz);

		r.reqDist2D = dist;
		r.maxDist = speed * dtSec + tolerance;

		outPos = req;

		if (dist <= r.maxDist || dist <= 1e-6f)
		{
			r.policy = SpeedPolicy::OK;
			return r;
		}

		const float inv = 1.0f / dist;
		const float nx = dx * inv;
		const float nz = dz * inv;

		outPos.set_x(cur.x() + nx * r.maxDist);
		outPos.set_z(cur.z() + nz * r.maxDist);

		// y는 navmesh가 진실이라 여기선 건드리지 말자
		outPos.set_y(req.y());

		// 회전/상태는 요청 유지 (C가 최종 확정)
		outPos.set_yaw(req.yaw());
		outPos.set_state(req.state());
		outPos.set_actionstate(req.actionstate());

		r.policy = SpeedPolicy::CLAMPED;
		return r;
	}
}
