#ifndef PRESSSWITCH_H
#define PRESSSWITCH_H


#include "objectmodifier.h"
#include "switch.h"

class MapFileI;
class MapFileO;
class RoomMap;
class DeltaFrame;
class GraphicsManager;

class PressSwitch: public Switch {
public:
    PressSwitch(GameObject* parent, int color, bool persistent, bool active);
    virtual ~PressSwitch();

	void make_str(std::string&);
    ModCode mod_code();
    void serialize(MapFileO& file);
    static void deserialize(MapFileI&, RoomMap*, GameObject*);

    void map_callback(RoomMap*, DeltaFrame*, MoveProcessor*);

    void check_send_signal(RoomMap*, DeltaFrame*);
    bool should_toggle(RoomMap*);

    void setup_on_put(RoomMap*, bool real);
    void cleanup_on_take(RoomMap*, bool real);

    void draw(GraphicsManager*, FPoint3);

    std::unique_ptr<ObjectModifier> duplicate(GameObject*, RoomMap*, DeltaFrame*);

protected:
    int color_;

    friend class ModifierTab;
};

#endif // PRESSSWITCH_H
