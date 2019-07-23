#include "stdafx.h"
#include "playingstate.h"

#include "graphicsmanager.h"

#include <filesystem>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "gameobject.h"

#include "gameobjectarray.h"
#include "delta.h"
#include "player.h"
#include "room.h"
#include "roommap.h"
#include "camera.h"
#include "moveprocessor.h"
#include "door.h"
#include "mapfile.h"
#include "car.h"

#include "common_constants.h"
#include "string_constants.h"

const std::unordered_map<int, Point3> MOVEMENT_KEYS {
    {GLFW_KEY_RIGHT, {1, 0, 0}},
    {GLFW_KEY_LEFT,  {-1,0, 0}},
    {GLFW_KEY_DOWN,  {0, 1, 0}},
    {GLFW_KEY_UP,    {0,-1, 0}},
};


PlayingState::PlayingState():
    GameState(), loaded_rooms_ {}, objs_ {std::make_unique<GameObjectArray>()},
    move_processor_ {}, room_ {}, player_ {},
    undo_stack_ {std::make_unique<UndoStack>(MAX_UNDO_DEPTH)} {}

PlayingState::~PlayingState() {}

void PlayingState::main_loop() {
    if (!move_processor_) {
        delta_frame_ = std::make_unique<DeltaFrame>();
    }
    handle_input();
    room_->draw(gfx_, player_, false, false);
    if (!move_processor_) {
        undo_stack_->push(std::move(delta_frame_));
    }
}

void PlayingState::handle_input() {
    static int input_cooldown = 0;
    static int undo_combo = 0;
    if (glfwGetKey(window_, GLFW_KEY_Z) == GLFW_PRESS) {
        if (input_cooldown == 0) {
            ++undo_combo;
            if (undo_combo < UNDO_COMBO_FIRST) {
                input_cooldown = UNDO_COOLDOWN_FIRST;
            } else if (undo_combo < UNDO_COMBO_SECOND) {
                input_cooldown = UNDO_COOLDOWN_SECOND;
            } else {
                input_cooldown = UNDO_COOLDOWN_FINAL;
            }
            if (move_processor_) {
                move_processor_->abort();
                move_processor_.reset(nullptr);
                delta_frame_->revert();
                delta_frame_ = std::make_unique<DeltaFrame>();
                room_->map()->reset_local_state();
                if (player_) {
                    room_->set_cam_pos(player_->pos_);
                }
            } else if (undo_stack_->non_empty()) {
                undo_stack_->pop();
                if (player_) {
                    room_->set_cam_pos(player_->pos_);
                }
            }
            return;
        }
    } else {
        undo_combo = 0;
    }
    bool ignore_input = false;
    if (input_cooldown > 0) {
        --input_cooldown;
        ignore_input = true;
    }
    // Ignore all other input if an animation is occurring
    if (move_processor_) {
        if (move_processor_->update()) {
            move_processor_.reset(nullptr);
            undo_stack_->push(std::move(delta_frame_));
            delta_frame_ = std::make_unique<DeltaFrame>();
        } else {
            return;
        }
    }
    if (ignore_input) {
        return;
    }
    RoomMap* room_map = room_->map();
    // TODO: Make a real "death" flag/state
    // Don't allow other input if player is "dead"
    if (!dynamic_cast<Player*>(room_map->view(player_->pos_))) {
        return;
    }
    for (auto p : MOVEMENT_KEYS) {
        if (glfwGetKey(window_, p.first) == GLFW_PRESS) {
            move_processor_ = std::make_unique<MoveProcessor>(this, room_map, delta_frame_.get(), true);
            // p.second == direction of movement
            if (!move_processor_->try_move(player_, p.second)) {
                move_processor_.reset(nullptr);
                return;
            }
            input_cooldown = MAX_COOLDOWN;
            Point3 pos_below;
            return;
        }
    }
	if (glfwGetKey(window_, GLFW_KEY_A) == GLFW_PRESS) {
		room_->camera()->change_rotation(-0.05f);
	}
	if (glfwGetKey(window_, GLFW_KEY_D) == GLFW_PRESS) {
		room_->camera()->change_rotation(0.05f);
	}
	if (glfwGetKey(window_, GLFW_KEY_X) == GLFW_PRESS) {
		player_->toggle_riding(room_map, delta_frame_.get());
		input_cooldown = MAX_COOLDOWN;
		return;
	}
    if (glfwGetKey(window_, GLFW_KEY_X) == GLFW_PRESS) {
        player_->toggle_riding(room_map, delta_frame_.get());
        input_cooldown = MAX_COOLDOWN;
        return;
    } else if (glfwGetKey(window_, GLFW_KEY_C) == GLFW_PRESS) {
        move_processor_ = std::make_unique<MoveProcessor>(this, room_map, delta_frame_.get(), true);
		if (move_processor_->color_change(player_)) {
			input_cooldown = MAX_COOLDOWN;
		}
		else {
			move_processor_.reset(nullptr);
		}
        return;
    }
}

Room* PlayingState::active_room() {
	return room_;
}

bool PlayingState::activate_room(Room* room) {
	return activate_room(room->name());
}

bool PlayingState::activate_room(const std::string& name) {
    if (!loaded_rooms_.count(name)) {
        if (!load_room(name)) {
            return false;
        }
    }
    room_ = loaded_rooms_[name].get();
    return true;
}

// TODO: load_room should be able to throw, maybe
bool PlayingState::load_room(const std::string& name) {
    // This will be much more complicated when save files are a thing
    std::string path = MAPS_TEMP + name + ".map";
    if (!std::filesystem::exists(path.c_str())) {
        path = MAPS_MAIN + name + ".map";
        if (!std::filesystem::exists(path.c_str())) {
            return false;
        }
    }
    MapFileI file {path};
    auto room = std::make_unique<Room>(name);
    room->load_from_file(*objs_, file);
    // Load dynamic component!
    room->map()->set_initial_state(false);
    loaded_rooms_[name] = std::move(room);
    return true;
}

bool PlayingState::can_use_door(Door* door, std::vector<DoorTravellingObj>& objs, Room** dest_room_ptr) {
    MapLocation* dest = door->dest();
    if (!loaded_rooms_.count(dest->name)) {
        load_room(dest->name);
    }
    Room* dest_room = loaded_rooms_[dest->name].get();
	*dest_room_ptr = dest_room;
    RoomMap* cur_map = room_->map();
    RoomMap* dest_map = dest_room->map();
    Point3 dest_pos_local = Point3{dest->pos} + Point3{dest_room->offset_pos_};
    for (auto& obj : objs) {
        obj.dest = dest_pos_local + obj.raw->pos_ - door->pos();
        if (dest_map->view(obj.dest)) {
            return false;
        }
    }
    return true;
}

void PlayingState::snap_camera_to_player() {
	room_->set_cam_pos(player_->pos_);
}

void PlayingState::init_player(Point3 pos) {
	RidingState rs = RidingState::Free;
	if (ColoredBlock* below = dynamic_cast<ColoredBlock*>(room_->map()->view({ pos.x, pos.y, pos.z - 1 }))) {
		if (dynamic_cast<Car*>(below->modifier())) {
			rs = RidingState::Riding;
		} else {
			rs = RidingState::Bound;
		}
	}
	auto player = std::make_unique<Player>(pos, rs);
	player_ = player.get();
	room_->map()->create(std::move(player), nullptr);
}