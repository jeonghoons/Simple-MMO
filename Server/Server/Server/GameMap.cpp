#include "pch.h"
#include "GameMap.h"



std::pair<int, int> GameMap::GetCellIndex(const PositionInfo& pos) const
{
	int x_index = static_cast<int>(pos.pos_x / CELL_SIZE);
	int y_index = static_cast<int>(pos.pos_y / CELL_SIZE);

	x_index = std::max(0, std::min(x_index, GRID_WIDTH - 1));
	y_index = std::max(0, std::min(y_index, GRID_HEIGHT - 1));

	return { x_index, y_index };
}

bool GameMap::UpdateObjectPosition(int objectId, const PositionInfo& new_pos)
{
    std::pair<int, int> new_cell = GetCellIndex(new_pos);

    std::pair<int, int> old_cell;
    bool isFirstEntry = (_currentCellIndices.count(objectId) == 0);
    if (isFirstEntry) {
        old_cell = { -1, -1 };
    }
    else {
        old_cell = _currentCellIndices.at(objectId);
    }

    // Cell이 변경되었거나, 처음 입장한 경우에만 갱신
    if (isFirstEntry || old_cell != new_cell)
    {
        // 이전 Cell에서 제거
        if (!isFirstEntry && old_cell.first >= 0 && old_cell.second >= 0) {
            // objectsIds -> object_ids (일관성 유지를 위해)
            _grid[old_cell.second][old_cell.first].objectsIds.erase(objectId);
        }

        // 새 Cell에 추가
        _grid[new_cell.second][new_cell.first].objectsIds.insert(objectId);

        // 현재 Cell 인덱스 갱신
        _currentCellIndices[objectId] = new_cell;
        return true;
    }

    return false;
}


unordered_set<int> GameMap::GetObjectIds(int objectId) const
{
    if (_currentCellIndices.count(objectId) == 0)
        return {};

    std::pair<int, int> center_cell_index = _currentCellIndices.at(objectId);
    std::unordered_set<int> candidate_ids;
    int center_x = center_cell_index.first;
    int center_y = center_cell_index.second;

    for (int y = center_y - VIEW_RANGE_CELLS; y <= center_y + VIEW_RANGE_CELLS; ++y) {
        for (int x = center_x - VIEW_RANGE_CELLS; x <= center_x + VIEW_RANGE_CELLS; ++x) {
            if (x >= 0 && x < GRID_WIDTH && y >= 0 && y < GRID_HEIGHT) {
                const auto& cell_ids = _grid[y][x].objectsIds;
                candidate_ids.insert(cell_ids.begin(), cell_ids.end());
            }
        }
    }
    return candidate_ids;
}

