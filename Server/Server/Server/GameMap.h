#pragma once

constexpr float MAP_WIDTH = 50.0f;
constexpr float MAP_HEIGHT = 50.0f;
constexpr int CELL_SIZE = 5;
constexpr int GRID_WIDTH = static_cast<int>(MAP_WIDTH / CELL_SIZE);
constexpr int GRID_HEIGHT = static_cast<int>(MAP_HEIGHT / CELL_SIZE);
const int VIEW_RANGE_CELLS = 1;

struct Cell
{
	unordered_set<int> objectsIds;
};

struct CellPos {
    int x, y;

    bool operator==(const CellPos& other) const { return x == other.x && y == other.y; }
    bool operator!=(const CellPos& other) const { return !(*this == other); }
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
    GameMap() : _grid(GRID_HEIGHT, vector<Cell>(GRID_WIDTH)) {}
    void Init(weak_ptr<Room> room) { _ownerRoom = room; }

public:
    CellPos GetCellPos(const PositionInfo& pos) const;
    bool UpdateObjectPosition(int objectId, const PositionInfo& currPos);
    unordered_set<int> GetObjectIds(int objectId) const;


    int ValidateMove(int objectId, const PositionInfo& new_pos) const;
    bool OutOfBounds(const PositionInfo& pos) const;
    bool CanMove(int objectId, const PositionInfo& new_pos) const;    

private:
    pair<int, int> GetTilePosition(const PositionInfo& pos) const;

private:
    weak_ptr<Room>             _ownerRoom;    
    vector<vector<Cell>>        _grid;
    unordered_map<int, CellPos>  _currentCellIndices;
};

