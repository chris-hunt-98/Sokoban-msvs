#include "stdafx.h"
#include "editorbasestate.h"

#include "common_constants.h"
#include "room.h"
#include "roommap.h"
#include "gameobject.h"
#include "window.h"


const std::unordered_map<int, Point3> EDITOR_MOVEMENT_KEYS {
    {GLFW_KEY_D, {1, 0, 0}},
    {GLFW_KEY_A, {-1,0, 0}},
    {GLFW_KEY_S, {0, 1, 0}},
    {GLFW_KEY_W, {0,-1, 0}},
    {GLFW_KEY_E, {0, 0, 1}},
    {GLFW_KEY_Q, {0, 0,-1}},
};


EditorBaseState::EditorBaseState(GameState* parent): GameState(parent),
ortho_cam_ {true}, one_layer_ {false}, keyboard_cooldown_ {0} {}

EditorBaseState::~EditorBaseState() {}

bool EditorBaseState::want_capture_keyboard() {
    return ImGui::GetIO().WantCaptureKeyboard;
}

bool EditorBaseState::want_capture_mouse() {
    return ImGui::GetIO().WantCaptureMouse;
}

Point3 EditorBaseState::get_pos_from_mouse(Point3 cam_pos) {
    double xpos, ypos;
    glfwGetCursorPos(window_->window_, &xpos, &ypos);
    double mesh_size_x = SCREEN_WIDTH / ORTHO_WIDTH;
    double mesh_size_y = SCREEN_HEIGHT / ORTHO_HEIGHT;
    if (xpos >= 0 && xpos < SCREEN_WIDTH && ypos >= 0 && ypos < SCREEN_HEIGHT) {
		int x_raw = (int)(xpos + mesh_size_x * cam_pos.x - (SCREEN_WIDTH - mesh_size_x) / 2);
		int y_raw = (int)(ypos + mesh_size_y * cam_pos.y - (SCREEN_HEIGHT - mesh_size_y) / 2);
		// Adjust for truncate-toward-zero division
		int x = (x_raw / mesh_size_x) - (x_raw < 0);
		int y = (y_raw / mesh_size_y) - (y_raw < 0);
        return {x, y, cam_pos.z};
    }
    return {};
}

void EditorBaseState::display_hover_pos_object(Point3 cam_pos, RoomMap* map) {
    Point3 mouse_pos = get_pos_from_mouse(cam_pos);
    ImGui::Text("Hover Pos: (%d,%d,%d)", mouse_pos.x, mouse_pos.y, mouse_pos.z);
	if (map->valid(mouse_pos)) {
		if (GameObject* obj = map->view(mouse_pos)) {
			ImGui::Text(obj->to_str().c_str());
		} else {
			ImGui::Text("Empty");
		}
	} else {
		ImGui::Text("Out of Bounds");
	}
    
}

void EditorBaseState::clamp_to_room(Point3& pos, Room* room) {
    RoomMap* cur_map = room->map();
    pos = {
        std::max(0, std::min(cur_map->width_ - 1, pos.x)),
        std::max(0, std::min(cur_map->height_ - 1, pos.y)),
        std::max(0, std::min(cur_map->depth_ - 1, pos.z))
    };
}

void EditorBaseState::handle_mouse_input(Point3 cam_pos, Room* room) {
    if (want_capture_mouse() || !ortho_cam_) {
        return;
    }
    Point3 mouse_pos = get_pos_from_mouse(cam_pos);
    if (!room->valid(mouse_pos)) {
        return;
    }
    if (glfwGetMouseButton(window_->window_, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        handle_left_click(mouse_pos);
    } else if (glfwGetMouseButton(window_->window_, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
        handle_right_click(mouse_pos);
    }
}

bool EditorBaseState::handle_keyboard_input(Point3& cam_pos, Room* room) {
    for (auto p : EDITOR_MOVEMENT_KEYS) {
        if (key_pressed(p.first)) {
            if (key_pressed(GLFW_KEY_LEFT_SHIFT)) {
                cam_pos += FAST_MAP_MOVE * p.second;
            } else {
                cam_pos += p.second;
            }
            clamp_to_room(cam_pos, room);
            return true;
        }
    }
    if (key_pressed(GLFW_KEY_C)) {
        ortho_cam_ = !ortho_cam_;
        return true;
    } else if (key_pressed(GLFW_KEY_F)) {
        one_layer_ = !one_layer_;
        return true;
	}
	return false;
}

