#ifndef MAPLAYER_H
#define MAPLAYER_H



#include <unordered_map>
#include <functional>

#include "common_enums.h"
#include "point.h"

class GameObject;
class DeltaFrame;
class RoomMap;
class MapFileO;
class GraphicsManager;
class GameObjectArray;

struct MapRect {
    int x;
    int y;
    int w;
    int h;
    bool contains(Point2);
};

using GameObjIDFunc = std::function<void(int)>;
using GameObjIDPosFunc = std::function<void(int, Point3)>;

class MapLayer {
public:
    MapLayer(RoomMap*, int z);
    virtual ~MapLayer() = 0;

    virtual int& at(Point2 pos) = 0;
    virtual MapCode type() = 0;

    virtual void apply_to_rect(MapRect, GameObjIDFunc&) = 0;
	virtual void apply_to_rect_with_pos(MapRect, GameObjIDPosFunc&) = 0;
    virtual void extend_by(int dx, int dy) = 0;
    virtual void shift_by(int dx, int dy, int dz) = 0;

protected:
    RoomMap* parent_map_;
	int z_;
};

class FullMapLayer: public MapLayer {
public:
    FullMapLayer(RoomMap*, int width, int height, int z);
    ~FullMapLayer();

    int& at(Point2 pos);
    MapCode type();

    void apply_to_rect(MapRect, GameObjIDFunc&);
	void apply_to_rect_with_pos(MapRect, GameObjIDPosFunc&);
    void extend_by(int dx, int dy);
    void shift_by(int dx, int dy, int dz);

private:
    std::vector<std::vector<int>> map_;
    int width_;
    int height_;
};


class SparseMapLayer: public MapLayer {
public:
    SparseMapLayer(RoomMap*, int z);
    ~SparseMapLayer();

    int& at(Point2 pos);
    MapCode type();

    void apply_to_rect(MapRect, GameObjIDFunc&);
	void apply_to_rect_with_pos(MapRect, GameObjIDPosFunc&);
    void extend_by(int dx, int dy);
    void shift_by(int dx, int dy, int dz);

private:
    std::unordered_map<Point2, int, Point2Hash> map_;
};

#endif // MAPLAYER_H