#ifndef MODIFIERTAB_H
#define MODIFIERTAB_H

#include "editortab.h"

class ColorCycle;

class ModifierTab: public EditorTab {
public:
    ModifierTab(EditorState*);
    ~ModifierTab();
    void init();
    void main_loop(EditorRoom*);
	bool handle_keyboard_input();
    void handle_left_click(EditorRoom*, Point3);
    void handle_right_click(EditorRoom*, Point3);

    void mod_tab_options(RoomMap*);
    void select_color_cycle(ColorCycle*);
};

#endif // MODIFIERTAB_H
