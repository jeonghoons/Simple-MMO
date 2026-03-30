#pragma once

constexpr float MAP_WIDTH = 30000.0f;
constexpr float MAP_HEIGHT = 30000.0f;
constexpr int CELL_SIZE = 30000;
constexpr int GRID_WIDTH = static_cast<int>(MAP_WIDTH / CELL_SIZE);
constexpr int GRID_HEIGHT = static_cast<int>(MAP_HEIGHT / CELL_SIZE);
const int VIEW_RANGE_CELLS = 1;

struct ViewUpdate {
    vector<int> entered;   // 새로 시야에 들어온 객체들
    vector<int> leaved; // 시야에서 사라진 객체들
};

struct Cell
{
    vector<int> objectIds;
};

struct CellPos {
    int x, y;

    bool operator==(const CellPos& other) const { return x == other.x && y == other.y; }
    bool operator!=(const CellPos& other) const { return !(*this == other); }
};

class Room;

class GameMap
{
public:
    GameMap() : _grid(GRID_HEIGHT, vector<Cell>(GRID_WIDTH)) {}
    void Init(weak_ptr<Room> room) { _ownerRoom = room; }

public:
    CellPos ToCellPos(const PositionInfo& pos) const;
    
    ViewUpdate EnterMap(int objectId, const PositionInfo& pos);
    ViewUpdate UpdateMap(int objectId, const PositionInfo& pos);
    ViewUpdate LeaveMap(int objectId);

    void CollectObject(CellPos pos, vector<int>& outList) const;
    vector<CellPos> GetNeighborCells(CellPos pos) const;

    bool OutOfBounds(const PositionInfo& pos) const;
    bool CanMove(int objectId, const PositionInfo& new_pos) const;    

private:
    pair<int, int> GetTilePosition(const PositionInfo& pos) const;

private:
    weak_ptr<Room>             _ownerRoom;    
    vector<vector<Cell>>        _grid;
    unordered_map<int, CellPos>  _id2CellPos;
};

