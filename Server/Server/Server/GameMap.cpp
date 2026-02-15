#include "pch.h"
#include "GameMap.h"


CellPos GameMap::GetCellPos(const PositionInfo& pos) const
{
    int x_index = static_cast<int>((pos.x + MAP_WIDTH / 2.f) / CELL_SIZE);
    int y_index = static_cast<int>((pos.y + MAP_HEIGHT / 2.f) / CELL_SIZE);

    x_index = clamp(x_index, 0, GRID_WIDTH - 1);
    y_index = clamp(y_index, 0, GRID_HEIGHT - 1);

	return { x_index, y_index };
}

bool GameMap::UpdateObjectPosition(int objectId, const PositionInfo& currPos)
{
    CellPos curr_cell = GetCellPos(currPos);
    auto it = _currentCellIndices.find(objectId);
    if (it != _currentCellIndices.end()) { 
        CellPos old_cell = it->second;

        if (old_cell == curr_cell) return false;

        _grid[old_cell.y][old_cell.x].objectsIds.erase(objectId);
        it->second = curr_cell;
    }
    else {
        _currentCellIndices.emplace(objectId, curr_cell);
    }

    _grid[curr_cell.y][curr_cell.x].objectsIds.insert(objectId);

    return true;
}

unordered_set<int> GameMap::GetObjectIds(int objectId) const
{
    auto it = _currentCellIndices.find(objectId);
    if (it == _currentCellIndices.end()) {
        return {};
    }

    unordered_set<int> candidate_ids;
    CellPos cell_pos = it->second;
    for (int y = cell_pos.y - VIEW_RANGE_CELLS; y <= cell_pos.y + VIEW_RANGE_CELLS; ++y) {
        for (int x = cell_pos.x - VIEW_RANGE_CELLS; x <= cell_pos.x + VIEW_RANGE_CELLS; ++x) {
            if (x < 0 || x >= GRID_WIDTH || y < 0 || y >= GRID_HEIGHT) continue;
            candidate_ids.insert(_grid[y][x].objectsIds.begin(), _grid[y][x].objectsIds.end());
        }
    }

    return candidate_ids;
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

    CellPos new_cell_index = GetCellPos(new_pos);
    const auto& candidate_ids = _grid[new_cell_index.y][new_cell_index.x].objectsIds;

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


std::pair<int, int> GameMap::GetTilePosition(const PositionInfo& pos) const
{
    int tile_x = static_cast<int>(floor(pos.x));
    int tile_y = static_cast<int>(floor(pos.y));
    return { tile_x, tile_y };
}

