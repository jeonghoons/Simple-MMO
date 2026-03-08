#include "pch.h"
#include "GameMap.h"


CellPos GameMap::ToCellPos(const PositionInfo& pos) const
{
    int x_index = static_cast<int>((pos.x + MAP_WIDTH / 2.f) / CELL_SIZE);
    int y_index = static_cast<int>((pos.y + MAP_HEIGHT / 2.f) / CELL_SIZE);

    x_index = clamp(x_index, 0, GRID_WIDTH - 1);
    y_index = clamp(y_index, 0, GRID_HEIGHT - 1);

	return { x_index, y_index };
}

ViewUpdate GameMap::EnterMap(int objectId, const PositionInfo& pos)
{
    CellPos cellPos = ToCellPos(pos);
    _grid[cellPos.y][cellPos.x].objectIds.push_back(objectId);
    _id2CellPos[objectId] = cellPos;

    // 처음 들어왔을 때는 주변 모든 객체가 '추가' 대상임
    ViewUpdate result;
    vector<CellPos> neighbors = GetNeighborCells(cellPos);
    for (const auto& pos : neighbors) {
        CollectObject(pos, result.entered);
    }
    return result;
}

ViewUpdate GameMap::UpdateMap(int objectId, const PositionInfo& pos)
{
    CellPos newPos = ToCellPos(pos);
    CellPos oldPos = _id2CellPos[objectId];

    if (newPos == oldPos) return {}; // 같은 셀이면 시야 변화 없음

    auto& oldVec = _grid[oldPos.y][oldPos.x].objectIds;
    auto it = find(oldVec.begin(), oldVec.end(), objectId);
    if (it != oldVec.end()) {
        *it = oldVec.back();
        oldVec.pop_back();
    }
    _grid[newPos.y][newPos.x].objectIds.push_back(objectId);
    _id2CellPos[objectId] = newPos;

    ViewUpdate result;
    vector<CellPos> old_view = GetNeighborCells(oldPos);
    vector<CellPos> new_view = GetNeighborCells(newPos);

    for (const auto& cell : new_view) {
        
        if (find(old_view.begin(), old_view.end(), cell) == old_view.end()) {
            CollectObject(cell, result.entered);
        }
    }

    for (const auto& cell : old_view) {
        if (find(new_view.begin(), new_view.end(), cell) == new_view.end()) {
            CollectObject(cell, result.leaved);
        }
    }

    return result;
}

ViewUpdate GameMap::LeaveMap(int objectId)
{
    auto itLoc = _id2CellPos.find(objectId);
    if (itLoc == _id2CellPos.end()) return {};

    CellPos lastPos = itLoc->second;

    // 그리드에서 제거
    auto& vec = _grid[lastPos.y][lastPos.x].objectIds;
    auto it = find(vec.begin(), vec.end(), objectId);
    if (it != vec.end()) {
        *it = vec.back();
        vec.pop_back();
    }

    // 나갈 때는 시야에 있던 모든 것이 '삭제' 대상
    ViewUpdate result;
    vector<CellPos> surrounding = GetNeighborCells(lastPos);
    for (const auto& cp : surrounding) {
        CollectObject(cp, result.leaved);
    }

    _id2CellPos.erase(itLoc);
    return result;
}



void GameMap::CollectObject(CellPos pos, vector<int>& outList) const
{
    const vector<int>& ids = _grid[pos.y][pos.x].objectIds;
    outList.insert(outList.end(), ids.begin(), ids.end());
}

vector<CellPos> GameMap::GetNeighborCells(CellPos pos) const
{
    vector<CellPos> cells;
    int cellCount = VIEW_RANGE_CELLS * 2 + 1;
    cells.reserve(cellCount * cellCount);

    for (int y = pos.y - VIEW_RANGE_CELLS; y <= pos.y + VIEW_RANGE_CELLS; ++y) {
        for (int x = pos.x - VIEW_RANGE_CELLS; x <= pos.x + VIEW_RANGE_CELLS; ++x) {
            if (x >= 0 && x < GRID_WIDTH && y >= 0 && y < GRID_HEIGHT) {
                cells.push_back({ x, y });
            }
        }
    }
    return cells;
}

bool GameMap::OutOfBounds(const PositionInfo& pos) const
{
    return pos.x < -(MAP_WIDTH /2) || pos.x >= (MAP_WIDTH / 2) ||
        pos.y < -(MAP_HEIGHT / 2) || pos.y >= (MAP_HEIGHT / 2);
}

bool GameMap::CanMove(int objectId, const PositionInfo& new_pos) const
{
    if (OutOfBounds(new_pos)) {
        return false;
    }

    return true;
}


std::pair<int, int> GameMap::GetTilePosition(const PositionInfo& pos) const
{
    int tile_x = static_cast<int>(floor(pos.x));
    int tile_y = static_cast<int>(floor(pos.y));
    return { tile_x, tile_y };
}

