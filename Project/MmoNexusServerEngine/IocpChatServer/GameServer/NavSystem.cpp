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

#ifndef NAV_DEBUG
#define NAV_DEBUG 0
#endif

#if NAV_DEBUG
#define NAV_LOG(x) do { std::cout << x << std::endl; } while(0)
#else
#define NAV_LOG(x) do {} while(0)
#endif

NavSystem::NavSystem()
{
	_navMesh = dtAllocNavMesh();
	_navQuery = dtAllocNavMeshQuery();

	// [Fix] 검색 범위 설정 (이게 없으면 발밑을 못 찾음)
	// (x=2m, y=4m, z=2m) 범위 내의 NavMesh를 찾는다.
	// y를 높게 잡는 이유는 언덕, 계단, 혹은 클라/서버 간 미세한 높이 오차를 커버하기 위함이다.
	_polyPickExt[0] = 2.0f;
	_polyPickExt[1] = 4.0f;
	_polyPickExt[2] = 2.0f;

	// [Init Filter] 필터 초기화
	_filter.setIncludeFlags(0xffff);
	_filter.setExcludeFlags(0);
}

NavSystem::~NavSystem()
{
	dtFreeNavMeshQuery(_navQuery);
	dtFreeNavMesh(_navMesh);
}

// [GigaChad Header] RecastDemo가 쓰는 비공식 헤더 정의
struct NavMeshSetHeader
{
	int magic;
	int version;
	int numTiles;
	dtNavMeshParams params; // Detour 표준 파라미터 포함
};

struct NavMeshTileHeader
{
	dtTileRef tileRef;
	int dataSize;
};

bool NavSystem::Load(const std::string& path)
{
	std::cout << " [NavSystem] Loading: " << path << std::endl;

	std::ifstream file(path, std::ios::binary);
	if (!file.is_open())
	{
		std::cout << " [Error] File Not Found!" << std::endl;
		return false;
	}

	// 1. 매직 넘버 확인 (Byte 단위 비교)
	char magicBuf[4];
	file.read(magicBuf, 4);

	// 파일 커서 원복
	file.seekg(0, std::ios::beg);

	// [Logic Fix] int 변환 없이 문자 그대로 비교 (TESM)
	bool isMset = (magicBuf[0] == 'T' && magicBuf[1] == 'E' && magicBuf[2] == 'S' && magicBuf[3] == 'M');

	// [Case 1] MSET 포맷 (RecastDemo 전용) -> 포장 뜯기
	if (isMset)
	{
		std::cout << " [Info] Detected 'MSET' format (TESM). Unpacking..." << std::endl;

		NavMeshSetHeader header;
		file.read((char*)&header, sizeof(NavMeshSetHeader));

		if (header.version != 1)
		{
			std::cout << " [Error] MSET Version mismatch! (Expected: 1, Got: " << header.version << ")" << std::endl;
			return false;
		}

		// 1. NavMesh를 'Params'로 초기화
		dtStatus status = _navMesh->init(&header.params);
		if (dtStatusFailed(status))
		{
			std::cout << " [Error] dtNavMesh::init(params) Failed! Status: " << status << std::endl;
			return false;
		}

		// 2. 타일 루프 돌면서 추가
		std::cout << " [Info] Loading " << header.numTiles << " tiles..." << std::endl;

		for (int i = 0; i < header.numTiles; ++i)
		{
			NavMeshTileHeader tileHeader;
			file.read((char*)&tileHeader, sizeof(NavMeshTileHeader));

			if (!tileHeader.tileRef || !tileHeader.dataSize)
			{
				std::cout << " [Warning] Invalid Tile Header at index " << i << std::endl;
				break;
			}

			// 타일 데이터 할당
			unsigned char* data = (unsigned char*)dtAlloc(tileHeader.dataSize, DT_ALLOC_PERM);
			if (!data)
			{
				std::cout << " [Error] Memory Alloc Failed for Tile " << i << std::endl;
				return false;
			}

			file.read((char*)data, tileHeader.dataSize);

			// 타일 추가 (소유권은 NavMesh로 넘어감 -> DT_TILE_FREE_DATA)
			// 주의: Solo Mesh라도 MSET 포맷 안에 있으면 addTile로 넣어야 함
			status = _navMesh->addTile(data, tileHeader.dataSize, DT_TILE_FREE_DATA, 0, nullptr);

			if (dtStatusFailed(status))
			{
				std::cout << " [Error] Failed to add tile " << i << " (Status: " << status << ")" << std::endl;
				dtFree(data);
				return false;
			}
		}
	}
	// [Case 2] Raw Detour 포맷 (DNAV) -> 기존 방식
	else
	{
		std::cout << " [Info] Detected Raw Detour format (Single Block)." << std::endl;

		file.seekg(0, std::ios::end);
		size_t size = file.tellg();
		file.seekg(0, std::ios::beg);

		unsigned char* data = (unsigned char*)dtAlloc(size, DT_ALLOC_PERM);
		if (!data) return false;

		file.read((char*)data, size);

		dtStatus status = _navMesh->init(data, (int)size, DT_TILE_FREE_DATA);
		if (dtStatusFailed(status))
		{
			std::cout << " [Error] Raw init Failed! Status: " << status << std::endl;
			std::cout << "   (Maybe wrong file format? Magic: " << magicBuf[0] << magicBuf[1] << magicBuf[2] << magicBuf[3] << ")" << std::endl;
			dtFree(data);
			return false;
		}
	}

	// 3. Query 초기화 (공통)
	dtStatus status = _navQuery->init(_navMesh, 2048);
	if (dtStatusFailed(status))
	{
		std::cout << " [Error] Query Init Failed!" << std::endl;
		return false;
	}

	// 그룹 초기화
	_polyGroups.clear();
	_nextGroupId = 1;

	std::cout << " [Success] NavMesh Loaded Perfectly!" << std::endl;
	return true;
}

bool NavSystem::ValidateMove(const Protocol::PositionInfo& current,
	const Protocol::PositionInfo& target,
	Protocol::PositionInfo& outAdjusted)
{
	float startPos[3] = { current.x(), current.y(), current.z() };
	float endPos[3] = { target.x(),  target.y(),  target.z() };

	dtPolyRef startRef = FindNearestPoly(startPos, _polyPickExt);
	if (!startRef)
	{
		NAV_LOG(" [Nav] OUTSIDE_NAV_DROP");
		return false;
	}

	float t = 0.0f;
	float hitNormal[3];
	dtPolyRef path[32];
	int pathCount = 0;

	_navQuery->raycast(startRef, startPos, endPos, &_filter, &t, hitNormal, path, &pathCount, 32);

	if (t >= 1.0f)
	{
		float finalY = endPos[1];
		if (ResolvePoint(endPos[0], endPos[1], endPos[2], finalY))
		{
			outAdjusted.set_x(endPos[0]);
			outAdjusted.set_y(finalY);
			outAdjusted.set_z(endPos[2]);
			outAdjusted.set_yaw(target.yaw());

			NAV_LOG(" [Nav] VALIDATE_OK");
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		float hitPos[3];
		dtVlerp(hitPos, startPos, endPos, t);

		float slidePos[3];
		dtPolyRef visited[16];
		int nVisited = 0;

		dtStatus status = _navQuery->moveAlongSurface(
			startRef, startPos, endPos, &_filter,
			slidePos, visited, &nVisited, 16);

		float slideY = slidePos[1];
		if (dtStatusSucceed(status) && nVisited > 0 &&
			ResolvePoint(slidePos[0], slidePos[1], slidePos[2], slideY))
		{
			outAdjusted.set_x(slidePos[0]);
			outAdjusted.set_y(slideY);
			outAdjusted.set_z(slidePos[2]);
			outAdjusted.set_yaw(target.yaw());

			NAV_LOG("🚧 [Nav] COLLIDE_SLIDE_OK");
			return true;
		}

		float hitY = hitPos[1];
		if (ResolvePoint(hitPos[0], hitPos[1], hitPos[2], hitY))
		{
			outAdjusted.set_x(hitPos[0]);
			outAdjusted.set_y(hitY);
			outAdjusted.set_z(hitPos[2]);
			outAdjusted.set_yaw(target.yaw());

			NAV_LOG(" [Nav] COLLIDE_SLIDE_FAIL_HITSNAP");
			return true;
		}

		outAdjusted.set_x(startPos[0]);
		outAdjusted.set_y(startPos[1]);
		outAdjusted.set_z(startPos[2]);
		outAdjusted.set_yaw(target.yaw());

		NAV_LOG(" [Nav] COLLIDE_FAIL_ROLLBACK");
		return true;
	}
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

bool NavSystem::RaycastNav(const Protocol::PositionInfo& start,
	const Protocol::PositionInfo& end,
	float& outT)
{
	outT = 0.0f;
	if (_navQuery == nullptr)
		return false;

	float startPos[3] = { start.x(), start.y(), start.z() };
	float endPos[3] = { end.x(),   end.y(),   end.z() };

	float startNearest[3];
	dtPolyRef startRef = FindNearestPoly(startPos, _polyPickExt, startNearest);
	if (!startRef)
		return false;

	float hitNormal[3];
	dtPolyRef path[32];
	int pathCount = 0;

	dtStatus status = _navQuery->raycast(
		startRef,
		startNearest,   //  시작점 nearest로 오차 흡수
		endPos,
		&_filter,
		&outT,
		hitNormal,
		path,
		&pathCount,
		32
	);

	return dtStatusSucceed(status);
}

static void CompressWaypoints(std::vector<Vector3>& pts, float minDist)
{
	if (pts.size() <= 1) return;

	const float minDistSqr = minDist * minDist;
	std::vector<Vector3> out;
	out.reserve(pts.size());

	out.push_back(pts[0]);
	for (size_t i = 1; i < pts.size(); ++i)
	{
		const Vector3& a = out.back();
		const Vector3& b = pts[i];
		const float dx = b.x - a.x;
		const float dy = b.y - a.y;
		const float dz = b.z - a.z;
		const float d2 = dx * dx + dy * dy + dz * dz;

		if (d2 >= minDistSqr)
			out.push_back(b);
	}

	//  마지막은 end 유지
	if (!out.empty())
		out.back() = pts.back();

	pts.swap(out);
}

bool NavSystem::FindPathWaypoints(const Protocol::PositionInfo& start,
	const Protocol::PositionInfo& end,
	std::vector<Vector3>& outWaypoints)
{
	outWaypoints.clear();
	if (_navQuery == nullptr)
		return false;

	{
		const float dx = end.x() - start.x();
		const float dy = end.y() - start.y();
		const float dz = end.z() - start.z();
		if ((dx * dx + dy * dy + dz * dz) < 1e-6f)
		{
			outWaypoints.emplace_back(end.x(), end.y(), end.z());
			return true;
		}
	}

	float startPos[3] = { start.x(), start.y(), start.z() };
	float endPos[3] = { end.x(),   end.y(),   end.z() };

	float startNearest[3];
	float endNearest[3];

	dtPolyRef startRef = FindNearestPoly(startPos, _polyPickExt, startNearest);
	dtPolyRef endRef = FindNearestPoly(endPos, _polyPickExt, endNearest);

	if (!startRef || !endRef)
		return false;

	// ===== 1) findPath (poly corridor) =====
	static constexpr int MAX_POLYS = 256;
	dtPolyRef polys[MAX_POLYS];
	int polyCount = 0;

	dtStatus status = _navQuery->findPath(
		startRef, endRef,
		startNearest, endNearest,
		&_filter,
		polys, &polyCount,
		MAX_POLYS
	);

	if (dtStatusFailed(status) || polyCount <= 0)
		return false;

	// ===== 2) findStraightPath (waypoints) =====
	static constexpr int MAX_STRAIGHT = 64;
	float straightPath[3 * MAX_STRAIGHT];
	unsigned char straightFlags[MAX_STRAIGHT];
	dtPolyRef straightPolys[MAX_STRAIGHT];
	int straightCount = 0;

	status = _navQuery->findStraightPath(
		startNearest,
		endNearest,
		polys,
		polyCount,
		straightPath,
		straightFlags,
		straightPolys,
		&straightCount,
		MAX_STRAIGHT,
		0
	);

	if (dtStatusFailed(status) || straightCount <= 0)
		return false;

	outWaypoints.reserve(straightCount);

	for (int i = 0; i < straightCount; ++i)
	{
		const float* p = &straightPath[i * 3];
		float y = p[1];

		//  높이 안정화 (polyHeight 우선)
		if (straightPolys[i] != 0)
		{
			float h = 0.0f;
			if (dtStatusSucceed(_navQuery->getPolyHeight(straightPolys[i], p, &h)))
				y = h;
			else
			{
				float snapY = y;
				if (ResolvePoint(p[0], p[1], p[2], snapY))
					y = snapY;
			}
		}
		else
		{
			float snapY = y;
			if (ResolvePoint(p[0], p[1], p[2], snapY))
				y = snapY;
		}

		outWaypoints.emplace_back(p[0], y, p[2]);
	}

	//  떨림/불필요한 점 방지 (5cm)
	CompressWaypoints(outWaypoints, 0.05f);

	return true;
}
