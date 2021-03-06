#ifndef MAPFILE_H
#define MAPFILE_H

#include "point.h"
#include "delta.h"

enum class MapCode {
	NONE = 0,
	Dimensions = 1, // The dimensions of the room as 1 byte integers
	Zone = 2, // Indicates which of the 36 zones the map is in
	ClearFlagRequirement = 3, // Indicates the number of clear flags to clear this room
	WallRuns = 4, // A (relatively) efficient encoding of walls
	OffsetPos = 5, // The position that (0,0) was at when the room was created
	Objects = 6, // Read in all map objects
	CameraRects = 7, // Get a camera context rectangle
	SnakeLink = 8, // Link two snakes (1 = Right, 2 = Down)
	DoorDest = 9, // Give a door a destination Map + Pos
	ThresholdSignaler = 10,
	ParitySignaler = 11,
	InitFlag = 12, // Indicates that the room has been initialized before
	GateBaseLocation = 13, // Indicates that a GateBody needs to be paired with its parent
	WallPositions = 14, // Initiates list of Wall Positions
	GlobalFlag = 15, // Indicates a global flag (only for flag-room synchronization purposes)
	DefaultPlayerPos = 16, // The position of the default starting player
	DefaultCarPos = 17, // The position of the default starting car (optional)
	ActivePlayerPos = 18, // The position of the active player
	FateSignaler = 19,
	FloorSignFlag = 20,
	DoorFlag = 21,
	PlayerCycle = 22,
	DeadObjs = 23,
	Deltas = 24,
	DoorRelationsFrozen = 25,
	ObjectsWithIDs = 26,
	PlayerDeath = 27,
	WallColorSpec = 28,
	BackgroundSpec = 29,
	End = 255,
};


enum class ObjRefCode {
	Tangible = 1,
	Inaccessible = 2,
	Null = 7,
};


class ColorCycle;

enum class ObjCode;
enum class ModCode;
enum class CameraCode;
enum class DeltaCode;
enum class Sticky;
enum class PlayerState;
enum class CarType;
enum class AnimationSignal;
enum class CauseOfDeath;
enum class WallColorType;
enum class BackgroundSpecType;
enum class BackgroundParticleType;

class GameObjectArray;

class MapFileI {
public:
    MapFileI(const std::filesystem::path& path);
    virtual ~MapFileI();
    void read(unsigned char* b, int n);

    unsigned char read_byte();
	unsigned int read_uint32();
	int read_int32();
    Point3 read_point3();
	Point3 read_spoint3();
    std::string read_str();
	std::string read_long_str();
	FrozenObject read_frozen_obj();

private:
    std::ifstream file_;
};

MapFileI& operator>>(MapFileI& f, int& v);
MapFileI& operator>>(MapFileI& f, bool& v);
MapFileI& operator>>(MapFileI& f, double& v);
MapFileI& operator>>(MapFileI& f, float& v);

MapFileI& operator>>(MapFileI& f, Point2& v);
MapFileI& operator>>(MapFileI& f, Point3& v);
MapFileI& operator>>(MapFileI& f, Point3_S16& v);
MapFileI& operator>>(MapFileI& f, FPoint3& v);
MapFileI& operator>>(MapFileI& f, IntRect& v);
MapFileI& operator>>(MapFileI& f, FloatRect& v);
MapFileI& operator>>(MapFileI& f, glm::vec4& v);

MapFileI& operator>>(MapFileI& f, ColorCycle& v);


class MapFileO {
public:
    MapFileO(const std::filesystem::path& path);
    ~MapFileO();

	void write_uint32(unsigned int);
	void write_int32(int);
	void write_spoint3(Point3);
	void write_long_str(std::string str);

    MapFileO& operator<<(unsigned char);
    MapFileO& operator<<(int);
    MapFileO& operator<<(unsigned int);
    MapFileO& operator<<(bool);
    MapFileO& operator<<(double);

    MapFileO& operator<<(Point2);
    MapFileO& operator<<(Point3);
    MapFileO& operator<<(Point3_S16);
    MapFileO& operator<<(FPoint3);
	MapFileO& operator<<(IntRect);
	MapFileO& operator<<(FloatRect);
	MapFileO& operator<<(glm::vec4);

    MapFileO& operator<<(std::string);
    MapFileO& operator<<(const ColorCycle&);

    MapFileO& operator<<(MapCode);
    MapFileO& operator<<(ObjCode);
    MapFileO& operator<<(ModCode);
	MapFileO& operator<<(DeltaCode);
    MapFileO& operator<<(CameraCode);
	MapFileO& operator<<(ObjRefCode);
    MapFileO& operator<<(Sticky);
    MapFileO& operator<<(PlayerState);
	MapFileO& operator<<(CarType);
	MapFileO& operator<<(AnimationSignal);
	MapFileO& operator<<(CauseOfDeath);
	MapFileO& operator<<(WallColorType);
	MapFileO& operator<<(BackgroundSpecType);
	MapFileO& operator<<(BackgroundParticleType);

private:
    std::ofstream file_;
};

#endif // MAPFILE_H
