#ifndef COMMON_ENUMS_H
#define COMMON_ENUMS_H

enum class ObjCode {
    NONE = 0,
    PushBlock = 1,
    SnakeBlock = 2,
    Wall = 3,
    Player = 4,
    GateBody = 5,
	ArtWall = 6,
};

enum class ModCode {
	NONE = 0,
	Door = 1,
	Car = 2,
	PressSwitch = 3,
	Gate = 4,
	AutoBlock = 5,
	PuppetBlock = 6,
	ClearFlag = 7,
	WorldResetKey = 8,
	PermanentSwitch = 9,
	FloorSign = 10,
	Incinerator = 11,
};

// This is useless for now, but we'll keep it anyway
enum class CameraCode {
    NONE = 0,
    General = 1,
	// Radial?
};

enum class Sticky {
    None = 0,
	// Stickiness checks
	WeakStick = 3,
	StrongStick = 4,
	Weak = 19,
	Strong = 36,
	All = 63,
	// Block levels
    WeakBlock = 1,
	SemiBlock = 3,
    StrongBlock = 6,
	// Snakes
	Snake = 8,
	WeakSnake = 24,
	StrongSnake = 40,
};

enum class CauseOfDeath {
	None = 0,
	Fallen = 1,
	Split = 2,
	Incinerated = 3,
	Voided = 4,
	Collided = 5,
};

Sticky operator &(Sticky a, Sticky b);

#endif // COMMON_ENUMS_H
