#include "pch.h"
#include "GameMap.h"



std::pair<int, int> GameMap::GetCellIndex(const PositionInfo& pos) const
{
	int x_index = static_cast<int>(pos.x / CELL_SIZE);
	int y_index = static_cast<int>(pos.y / CELL_SIZE);

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

bool GameMap::OutOfBounds(const PositionInfo& pos) const
{
    return pos.x < 0.f || pos.x >= MAP_WIDTH || pos.y < 0.f || pos.y >= MAP_HEIGHT;;
}

bool GameMap::CanMove(int objectId, const PositionInfo& new_pos) const
{
    if (OutOfBounds(new_pos)) {
        return false;
    }
}

int GameMap::ValidateMove(int objectId, const PositionInfo& new_pos) const
{
    shared_ptr<Room> ownerRoom = _ownerRoom.lock();
    if (!ownerRoom) {
        return static_cast<int>(MoveResult::Error);
    }

    if (OutOfBounds(new_pos)) {
        return static_cast<int>(MoveResult::OutOfBounds);
    }

    std::pair<int, int> new_tile = GetTilePosition(new_pos);
    const int new_tile_x = new_tile.first;
    const int new_tile_y = new_tile.second;

    std::pair<int, int> new_cell_index = GetCellIndex(new_pos);
    const auto& candidate_ids = _grid[new_cell_index.second][new_cell_index.first].objectsIds;

    for (int target_id : candidate_ids) {
        if (target_id == objectId) continue;

        if (std::optional<PositionInfo> target_pos = ownerRoom->GetObjectPosition(target_id)) {

            std::pair<int, int> target_tile = GetTilePosition(*target_pos);

            
            if (target_tile.first == new_tile_x && target_tile.second == new_tile_y) {
                return target_id; 
            }
        }
    }

    return static_cast<int>(MoveResult::Validate);
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

std::pair<int, int> GameMap::GetTilePosition(const PositionInfo& pos) const
{
    int tile_x = static_cast<int>(std::round(pos.x));
    int tile_y = static_cast<int>(std::round(pos.y));
    return { tile_x, tile_y };
}

