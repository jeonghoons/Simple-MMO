#pragma once
#include "pch.h"
#include "DetourNavMesh.h"
#include "DetourNavMeshQuery.h"

struct NavMeshSetHeader {
    int magic;
    int version;
    int numTiles;
    dtNavMeshParams params;
};

struct NavMeshTileHeader {
    dtTileRef tileRef;
    int dataSize;
};

class NavmeshManager
{
public:
	NavmeshManager();
	~NavmeshManager();

	// 네비메시 로드
	bool LoadNavMesh(const std::string& path, float& outMinX, float& outMaxX, float& outMinY, float& outMaxY);

	// 맵 바깥인지 확인
	bool IsOutOfBounds(const PositionInfo& pos);

	// 시작점에서 목적지까지 장애물 없이 갈 수 있는지 레이캐스트 검증 (핵심 방어 로직)
	bool CanMove(const PositionInfo& startPos, const PositionInfo& destPos);

	// 길 찾기 (A* 알고리즘)
	bool FindPath(const PositionInfo& startPos, const PositionInfo& destPos, std::vector<PositionInfo>& outPath);
	
	bool RayCast(const PositionInfo& startPos, const PositionInfo& destPos);
private:
	// 언리얼(Z-up) <-> Detour(Y-up) 좌표 변환 유틸리티
	void UeToDetour(const PositionInfo& uePos, float* detourPos);
	void DetourToUe(const float* detourPos, PositionInfo& uePos);

private:
	dtNavMesh* _navMesh = nullptr;
	dtNavMeshQuery* _navQuery = nullptr;
	dtQueryFilter	_filter;
};

