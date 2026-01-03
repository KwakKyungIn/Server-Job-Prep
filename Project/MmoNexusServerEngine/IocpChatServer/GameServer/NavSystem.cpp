#include "pch.h"
#include "NavSystem.h"
#include <fstream>
#include <queue>
#include <set>
#include <iostream>

// [Fix 1] Detour 필수 헤더 명시적 포함
#include "DetourNavMesh.h"
#include "DetourNavMeshQuery.h"
#include "DetourCommon.h" 

NavSystem::NavSystem()
{
	_navMesh = dtAllocNavMesh();
	_navQuery = dtAllocNavMeshQuery();

	// [Init Filter] 필터 초기화
	_filter.setIncludeFlags(0xffff);
	_filter.setExcludeFlags(0);
}

NavSystem::~NavSystem()
{
	dtFreeNavMeshQuery(_navQuery);
	dtFreeNavMesh(_navMesh);
}

bool NavSystem::Load(const std::string& path)
{
	std::ifstream file(path, std::ios::binary);
	if (!file.is_open()) return false;

	file.seekg(0, std::ios::end);
	size_t size = file.tellg();
	file.seekg(0, std::ios::beg);

	char* data = new char[size];
	file.read(data, size);
	file.close();

	dtStatus status = _navMesh->init((unsigned char*)data, (int)size, DT_TILE_FREE_DATA);
	if (dtStatusFailed(status))
	{
		delete[] data;
		return false;
	}

	status = _navQuery->init(_navMesh, 2048);
	if (dtStatusFailed(status)) return false;

	// [Reset] 로드 시 그룹 정보 초기화
	_polyGroups.clear();
	_nextGroupId = 1;

	return true;
}


bool NavSystem::ValidateMove(const Protocol::PositionInfo& current, const Protocol::PositionInfo& target, Protocol::PositionInfo& outAdjusted)
{
	float startPos[3] = { current.x(), current.y(), current.z() };
	float endPos[3] = { target.x(), target.y(), target.z() };

	dtPolyRef startRef = FindNearestPoly(startPos, _polyPickExt);
	if (!startRef) return false;

	float t = 0.0f;
	float hitNormal[3];
	dtPolyRef path[32];
	int pathCount = 0;

	// MaxPath 32 설정
	_navQuery->raycast(startRef, startPos, endPos, &_filter,
		&t, hitNormal, path, &pathCount, 32);

	if (t < 1.0f) // 벽 충돌 발생
	{
		// 1. 충돌 지점(x, z) 계산
		float hitX = startPos[0] + (endPos[0] - startPos[0]) * t;
		float hitZ = startPos[2] + (endPos[2] - startPos[2]) * t;

		// 2. 높이(Y) 보정: 충돌 지점의 NavMesh 높이를 구한다.
		// 단순히 선형 보간하면 공중에 뜨거나 땅에 박힐 수 있음.
		float hitY = startPos[1] + (endPos[1] - startPos[1]) * t; // 임시 높이
		float correctedY;

		if (ResolvePoint(hitX, hitY, hitZ, correctedY))
		{
			// 성공 시 스냅된 위치로 설정
			outAdjusted.set_x(hitX);
			outAdjusted.set_y(correctedY);
			outAdjusted.set_z(hitZ);
		}
		else
		{
			// [Safety] NavMesh 위를 못 찾으면 차라리 시작 위치로 롤백 (끼임 방지)
			outAdjusted.set_x(startPos[0]);
			outAdjusted.set_y(startPos[1]);
			outAdjusted.set_z(startPos[2]);
		}
	}
	else
	{
		// 이동 성공
		float finalY;
		ResolvePoint(endPos[0], endPos[1], endPos[2], finalY);

		outAdjusted.set_x(endPos[0]);
		outAdjusted.set_y(finalY);
		outAdjusted.set_z(endPos[2]);
	}

	outAdjusted.set_yaw(target.yaw());
	return true;
}
dtPolyRef NavSystem::FindNearestPoly(const float* center, const float* extents, float* nearestPt)
{
	dtPolyRef ref = 0;
	float pt[3];
	_navQuery->findNearestPoly(center, extents, &_filter, &ref, pt);

	if (nearestPt && ref)
	{
		dtVcopy(nearestPt, pt);
	}
	return ref;
}

bool NavSystem::ResolvePoint(float x, float y, float z, float& outY)
{
	float pos[3] = { x, y, z };
	float result[3];
	dtPolyRef ref = FindNearestPoly(pos, _polyPickExt, result);

	if (ref)
	{
		outY = result[1];
		return true;
	}
	return false;
}

// [GigaChad Solution] Lazy Flood Fill
// getTile 접근 없이, 필요한 폴리곤 그룹만 그때그때 묶어서 캐싱한다.
uint32 NavSystem::GetConnectivityId(float x, float y, float z)
{
	float pos[3] = { x, y, z };
	dtPolyRef startRef = FindNearestPoly(pos, _polyPickExt);

	if (startRef == 0) return 0; // NavMesh 위가 아님

	// 1. 캐시 확인
	auto it = _polyGroups.find(startRef);
	if (it != _polyGroups.end())
	{
		return it->second;
	}

	// 2. Lazy Flood Fill
	// [Fix] static 변수 제거 -> 멤버 변수 _nextGroupId 사용 (멀티스레드/멀티맵 안전)
	uint32 newGroupId = _nextGroupId++;

	std::queue<dtPolyRef> q;
	q.push(startRef);
	_polyGroups[startRef] = newGroupId;

	while (!q.empty())
	{
		dtPolyRef currRef = q.front();
		q.pop();

		const dtMeshTile* currTile = 0;
		const dtPoly* currPoly = 0;
		_navMesh->getTileAndPolyByRef(currRef, &currTile, &currPoly);

		if (!currTile || !currPoly) continue;

		unsigned int linkIdx = currPoly->firstLink;
		while (linkIdx != DT_NULL_LINK)
		{
			const dtLink& link = currTile->links[linkIdx];
			if (link.ref != 0)
			{
				if (_polyGroups.find(link.ref) == _polyGroups.end())
				{
					_polyGroups[link.ref] = newGroupId;
					q.push(link.ref);
				}
			}
			linkIdx = link.next;
		}
	}

	return newGroupId;
}