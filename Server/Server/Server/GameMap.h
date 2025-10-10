#pragma once

const int MAP_WIDTH = 32;
const int MAP_HEIGHT = 32;
const int CELL_SIZE = 4;
const float VIEW_RANGE = 5.f;

struct Cell
{
	unordered_set<int> objectsIds;
};

class GameMap
{
public:
    GameMap() : _grid(GRID_HEIGHT, std::vector<Cell>(GRID_WIDTH)) {}
    std::pair<int, int> GetCellIndex(const PositionInfo& pos) const;

    bool UpdateObjectPosition(int objectId, const PositionInfo& new_pos);

    unordered_set<int> GetAOIObjectIds(int objectId) const;

   
private:
    

    const int GRID_WIDTH = MAP_WIDTH / CELL_SIZE;
    const int GRID_HEIGHT = MAP_HEIGHT / CELL_SIZE;
    
    const int VIEW_RANGE_CELLS = 1;

    vector<std::vector<Cell>> _grid;
    unordered_map<int, std::pair<int, int>> _currentCellIndices;
};

