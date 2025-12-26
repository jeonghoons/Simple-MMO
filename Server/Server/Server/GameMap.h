#pragma once

const int MAP_WIDTH = 100;
const int MAP_HEIGHT = 100;
const int CELL_SIZE = 5;
const float VIEW_RANGE = CELL_SIZE;
const int VIEW_RANGE_CELLS = 20;

struct Cell
{
	unordered_set<int> objectsIds;
};

enum MoveResult
{
    Validate = -1, // ok
    OutOfBounds = -2, // stay
    Error = -3, // error
};

class Room;

class GameMap
{
public:
    GameMap() : _grid(GRID_HEIGHT, std::vector<Cell>(GRID_WIDTH)) {}
    void Init(weak_ptr<Room> room) { _ownerRoom = room; }

    std::pair<int, int> GetCellIndex(const PositionInfo& pos) const;
    bool UpdateObjectPosition(int objectId, const PositionInfo& new_pos);

    int ValidateMove(int objectId, const PositionInfo& new_pos) const;
    bool OutOfBounds(const PositionInfo& pos) const;
    bool CanMove(int objectId, const PositionInfo& new_pos) const;


    unordered_set<int> GetObjectIds(int objectId) const;

private:
    std::pair<int, int> GetTilePosition(const PositionInfo& pos) const;

private:
    weak_ptr<Room>          _ownerRoom;
    const int GRID_WIDTH = MAP_WIDTH / CELL_SIZE;
    const int GRID_HEIGHT = MAP_HEIGHT / CELL_SIZE;
    
    vector<std::vector<Cell>> _grid;
    unordered_map<int, std::pair<int, int>> _currentCellIndices;
};

