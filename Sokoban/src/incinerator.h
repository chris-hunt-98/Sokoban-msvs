#pragma once

#include "switchable.h"

class Incinerator : public Switchable {
public:
	Incinerator(GameObject* parent, int count, bool persistent, bool def, bool active);
	~Incinerator();

	void make_str(std::string&);
	ModCode mod_code();
	void serialize(MapFileO& file);
	static void deserialize(MapFileI&, RoomMap*, GameObject*);

	bool can_set_state(bool state, RoomMap* map);
	void map_callback(RoomMap*, DeltaFrame*, MoveProcessor*);
	void apply_state_change(RoomMap*, DeltaFrame*, MoveProcessor*);

	void setup_on_put(RoomMap*, DeltaFrame*, bool real);
	void cleanup_on_take(RoomMap*, DeltaFrame*, bool real);
	void destroy(MoveProcessor* mp, CauseOfDeath);
	void signal_animation(AnimationManager*, DeltaFrame*);

	void draw(GraphicsManager*, FPoint3);

	std::unique_ptr<ObjectModifier> duplicate(GameObject*, RoomMap*, DeltaFrame*);
};

