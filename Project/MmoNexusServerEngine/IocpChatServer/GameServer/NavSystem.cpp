#include "pch.h"
#include "NavSystem.h"
#include <fstream>
#include <queue>
#include <set>
#include <iostream>

// Detour 라이브러리 사용을 위한 헤더
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
	// Detour NavMesh 객체 및 쿼리 객체 할당
	_navMesh = dtAllocNavMesh();
	_navQuery = dtAllocNavMeshQuery();

	// NavMesh 폴리곤을 찾을 때 사용할 검색 범위 설정
	// x, z는 평면 범위, y는 높이 범위
	// 서버랑 클라 간 미세한 좌표 오차나 경사로를 커버하기 위해 y값을 4.0으로 좀 크게 잡음
	_polyPickExt[0] = 2.0f;
	_polyPickExt[1] = 4.0f;
	_polyPickExt[2] = 2.0f;

	// 기본 필터 설정 (모든 플래그 포함, 제외 없음)
	_filter.setIncludeFlags(0xffff);
	_filter.setExcludeFlags(0);
}

NavSystem::~NavSystem()
{
	dtFreeNavMeshQuery(_navQuery);
	dtFreeNavMesh(_navMesh);
}

// RecastDemo 툴에서 사용하는 커스텀 헤더 구조체
// 파일 앞부분의 매직 넘버랑 버전 등을 확인하기 위함
struct NavMeshSetHeader
{
	int magic;
	int version;
	int numTiles;
	dtNavMeshParams params; // Detour 네비메쉬 설정 파라미터
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

	// 파일 앞 4바이트 읽어서 포맷 확인
	char magicBuf[4];
	file.read(magicBuf, 4);

	// 확인했으니 커서는 다시 처음으로 돌려둠
	file.seekg(0, std::ios::beg);

	// 매직 넘버가 TESM인지 확인 (RecastDemo MSET 포맷)
	bool isMset = (magicBuf[0] == 'T' && magicBuf[1] == 'E' && magicBuf[2] == 'S' && magicBuf[3] == 'M');

	// Case 1: MSET 포맷인 경우 (타일 여러 개가 묶여있는 구조)
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

		// 헤더에 있는 파라미터로 NavMesh 초기화
		dtStatus status = _navMesh->init(&header.params);
		if (dtStatusFailed(status))
		{
			std::cout << " [Error] dtNavMesh::init(params) Failed! Status: " << status << std::endl;
			return false;
		}

		// 타일 개수만큼 루프 돌면서 데이터 로드 및 추가
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

			// 타일 데이터 담을 메모리 할당
			unsigned char* data = (unsigned char*)dtAlloc(tileHeader.dataSize, DT_ALLOC_PERM);
			if (!data)
			{
				std::cout << " [Error] Memory Alloc Failed for Tile " << i << std::endl;
				return false;
			}

			file.read((char*)data, tileHeader.dataSize);

			// 읽어온 타일 데이터를 NavMesh에 추가
			// DT_TILE_FREE_DATA 플래그를 주면 NavMesh가 나중에 알아서 메모리 해제함
			status = _navMesh->addTile(data, tileHeader.dataSize, DT_TILE_FREE_DATA, 0, nullptr);

			if (dtStatusFailed(status))
			{
				std::cout << " [Error] Failed to add tile " << i << " (Status: " << status << ")" << std::endl;
				dtFree(data);
				return false;
			}
		}
	}
	// Case 2: 일반 Detour Raw 포맷인 경우 (단일 블록)
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

	// NavMeshQuery 초기화 (최대 노드 수 2048)
	dtStatus status = _navQuery->init(_navMesh, 2048);
	if (dtStatusFailed(status))
	{
		std::cout << " [Error] Query Init Failed!" << std::endl;
		return false;
	}

	// 폴리곤 그룹 캐시 초기화
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

	// 시작점이 네비메쉬 위에 있는지 확인
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

	// 시작점에서 목표점까지 레이캐스트 발사
	_navQuery->raycast(startRef, startPos, endPos, &_filter, &t, hitNormal, path, &pathCount, 32);

	// t >= 1.0f이면 중간에 막히는 곳 없이 끝까지 갈 수 있다는 뜻
	if (t >= 1.0f)
	{
		float finalY = endPos[1];
		// 도착점의 높이를 네비메쉬 높이로 보정
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
		// 중간에 벽에 부딪힌 경우
		float hitPos[3];
		dtVlerp(hitPos, startPos, endPos, t); // 부딪힌 지점 좌표 계산

		float slidePos[3];
		dtPolyRef visited[16];
		int nVisited = 0;

		// 벽에 막혔을 때 멈추지 않고 표면을 따라 미끄러지도록 처리 (Sliding)
		dtStatus status = _navQuery->moveAlongSurface(
			startRef, startPos, endPos, &_filter,
			slidePos, visited, &nVisited, 16);

		float slideY = slidePos[1];
		// 슬라이딩 성공 시 해당 위치로 보정
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

		// 슬라이딩 실패 시 그냥 부딪힌 지점으로 스냅
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

		// 다 실패하면 그냥 시작 위치로 롤백
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
	// 지정된 범위 내에서 가장 가까운 폴리곤 검색
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
		outY = result[1]; // 네비메쉬 상의 높이값(Y) 반환
		return true;
	}
	return false;
}

// Lazy Flood Fill 구현
// 맵 전체를 미리 계산하지 않고, 요청이 들어왔을 때 해당 구역만 탐색해서 ID를 부여함
// 성능 최적화를 위해 캐싱(polyGroups) 사용
uint32 NavSystem::GetConnectivityId(float x, float y, float z)
{
	float pos[3] = { x, y, z };
	dtPolyRef startRef = FindNearestPoly(pos, _polyPickExt);

	if (startRef == 0) return 0; // 네비메쉬 위가 아님

	// 1. 이미 계산된 폴리곤이면 캐시된 ID 반환
	auto it = _polyGroups.find(startRef);
	if (it != _polyGroups.end())
	{
		return it->second;
	}

	// 2. 처음 방문하는 곳이면 BFS(Flood Fill) 돌려서 연결된 모든 폴리곤을 같은 그룹으로 묶음
	uint32 newGroupId = _nextGroupId++;

	Queue<dtPolyRef> q;
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

		// 인접한 폴리곤들을 순회
		unsigned int linkIdx = currPoly->firstLink;
		while (linkIdx != DT_NULL_LINK)
		{
			const dtLink& link = currTile->links[linkIdx];
			if (link.ref != 0)
			{
				// 아직 방문 안 한 폴리곤이면 큐에 넣고 그룹 ID 할당
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

	// 시야 검사(Line of Sight)와 비슷함
	// 경로상에 장애물이 있는지 확인
	dtStatus status = _navQuery->raycast(
		startRef,
		startNearest,   // 시작점을 네비메쉬 위로 보정한 좌표 사용
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

// 너무 가까운 웨이포인트들은 하나로 합쳐서 최적화
static void CompressWaypoints(Vector<Vector3>& pts, float minDist)
{
	if (pts.size() <= 1) return;

	const float minDistSqr = minDist * minDist;
	Vector<Vector3> out;
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

		// 최소 거리 이상일 때만 추가
		if (d2 >= minDistSqr)
			out.push_back(b);
	}

	// 도착점은 중요하니까 무조건 유지
	if (!out.empty())
		out.back() = pts.back();

	pts.swap(out);
}

bool NavSystem::FindPathWaypoints(const Protocol::PositionInfo& start,
	const Protocol::PositionInfo& end,
	Vector<Vector3>& outWaypoints)
{
	outWaypoints.clear();
	if (_navQuery == nullptr)
		return false;

	// 시작점과 끝점이 거의 같으면 그냥 끝점 하나 넣고 리턴
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

	// 시작/끝 위치가 네비메쉬 위의 어디에 있는지 찾음
	dtPolyRef startRef = FindNearestPoly(startPos, _polyPickExt, startNearest);
	dtPolyRef endRef = FindNearestPoly(endPos, _polyPickExt, endNearest);

	if (!startRef || !endRef)
		return false;

	// 1단계: 폴리곤 경로 찾기 (findPath)
	// 시작 폴리곤부터 끝 폴리곤까지 연결된 폴리곤들의 리스트를 구함
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

	// 2단계: 스트레이트 패스 구하기 (findStraightPath)
	// 폴리곤 리스트 내부를 통과하는 실제 꺾이는 지점(Waypoints)들을 추출
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

	// 구해진 경로점들의 높이 값을 보정해서 리스트에 담음
	for (int i = 0; i < straightCount; ++i)
	{
		const float* p = &straightPath[i * 3];
		float y = p[1];

		// 해당 경로점이 속한 폴리곤의 정확한 높이 값을 가져오려고 시도
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

	// 3단계: 경로 압축
	// 너무 자잘하게 찍힌 포인트들은 제거해서 데이터 전송량 줄임 (5cm 기준)
	CompressWaypoints(outWaypoints, 0.05f);

	return true;
}