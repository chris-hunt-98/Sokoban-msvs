#include "stdafx.h"
#include "moveprocessor.h"

#include "common_constants.h"

#include "gameobject.h"
#include "player.h"
#include "gatebody.h"
#include "delta.h"
#include "room.h"
#include "roommap.h"
#include "door.h"
#include "car.h"

#include "snakeblock.h"

#include "playingstate.h"

#include "horizontalstepprocessor.h"
#include "fallstepprocessor.h"

MoveProcessor::MoveProcessor(PlayingState* playing_state, RoomMap* room_map, DeltaFrame* delta_frame, bool animated) :
	fall_check_{}, moving_blocks_{},
	playing_state_{ playing_state }, map_{ room_map }, delta_frame_{ delta_frame },
	entry_door_{}, door_travelling_objs_{}, dest_room_{},
	frames_{ 0 }, state_{}, door_state_{},
	animated_{ animated } {}

MoveProcessor::~MoveProcessor() {}

bool MoveProcessor::try_move(Player* player, Point3 dir) {
	if (player->state_ == RidingState::Bound) {
		move_bound(player, dir);
	} else {
		move_general(dir);
	}
	if (moving_blocks_.empty()) {
		return false;
	}
	state_ = MoveStep::Horizontal;
	frames_ = HORIZONTAL_MOVEMENT_FRAMES - SWITCH_RESPONSE_FRAMES;
	return true;
}

void MoveProcessor::move_bound(Player* player, Point3 dir) {
	// When Player is Bound, no other agents move
	if (map_->view(player->shifted_pos(dir))) {
		return;
	}
	// If the player is bound, it's on top of a block!
	auto* car = dynamic_cast<ColoredBlock*>(map_->view(player->shifted_pos({ 0,0,-1 })));
	auto* adj = dynamic_cast<ColoredBlock*>(map_->view(car->shifted_pos(dir)));
	if (adj && car->color() == adj->color()) {
		map_->take(player);
		player->set_linear_animation(dir);
		delta_frame_->push(std::make_unique<MotionDelta>(player, dir, map_));
		moving_blocks_.push_back(player);
		player->shift_pos_from_animation();
		map_->put(player);
	}
}

void MoveProcessor::move_general(Point3 dir) {
	HorizontalStepProcessor(map_, delta_frame_, dir, fall_check_, moving_blocks_).run();
}

bool MoveProcessor::update() {
	if (--frames_ <= 0) {
		switch (state_) {
		case MoveStep::Horizontal:
			// Even if no switch checks occur, the next frame chunk
			// cannot be skipped - horizontal animation must finish.
			perform_switch_checks(false);
			break;
		case MoveStep::PreFallSwitch:
		case MoveStep::ColorChange:
		case MoveStep::DoorMove:
		case MoveStep::PostDoorFallCheck:
			try_fall_step();
			// If nothing happens, skip the next forced wait.
			perform_switch_checks(true);
			break;
		default:
			break;
		}
	}
	// TODO: move this responsibility to a new class
	for (GameObject* block : moving_blocks_) {
		block->update_animation();
	}
	if (state_ == MoveStep::Done) {

	}
	//update_gate_transitions();
	return frames_ <= 0;
}

void MoveProcessor::abort() {
	// TODO: Pute this in the new animation managing class
	for (GameObject* block : moving_blocks_) {
		block->reset_animation();
	}
	/*for (auto& p : gate_transitions_) {
		p.first->reset_state_animation();
	}*/
}

// Returns whether a color change occured
bool MoveProcessor::color_change(Player* player) {
	// TODO: make this more general (in particular, be careful with the fall check!)
	Car* car = player->get_car(map_, false);
	if (!(car && car->cycle_color(false))) {
		return false;
	}
	if (auto snake = dynamic_cast<SnakeBlock*>(car->parent_)) {
		snake->update_links_color(map_, delta_frame_);
	}
	state_ = MoveStep::ColorChange;
	frames_ = 4;
	frames_ = COLOR_CHANGE_MOVEMENT_FRAMES;
	delta_frame_->push(std::make_unique<ColorChangeDelta>(car, true));
	add_neighbors_to_fall_check(car->parent_);
	return true;
}

void MoveProcessor::add_neighbors_to_fall_check(GameObject* obj) {
	fall_check_.push_back(obj);
	for (Point3 d : DIRECTIONS) {
		if (GameObject* adj = map_->view(obj->shifted_pos(d))) {
			fall_check_.push_back(adj);
		}
	}
}

void MoveProcessor::try_fall_step() {
	moving_blocks_.clear();
	if (!fall_check_.empty()) {
		FallStepProcessor(map_, delta_frame_, std::move(fall_check_)).run();
		fall_check_.clear();
	}
}

void MoveProcessor::perform_switch_checks(bool skippable) {
	delta_frame_->reset_changed();
	map_->alert_activated_listeners(delta_frame_, this);
	map_->reset_local_state();
	map_->check_signalers(delta_frame_, this);
	if (!skippable || delta_frame_->changed()) {
		state_ = MoveStep::PreFallSwitch;
		frames_ = FALL_MOVEMENT_FRAMES;
	} else {
		state_ = MoveStep::Done;
		switch (door_state_) {
		case DoorState::None:
			break;
		case DoorState::AwaitingEntry:
			try_door_entry();
			break;
		case DoorState::AwaitingIntExit:
			try_int_door_exit();
			break;
		case DoorState::AwaitingExtExit:
			ext_door_exit();
			break;
		case DoorState::AwaitingUnentry:
			try_door_unentry();
			break;
		}
	}
}

void MoveProcessor::plan_door_move(Door* door) {
	if (door_state_ == DoorState::None && door->usable()) {
		// TODO: make this more general later
		// Also, it should probably be the responsibility of the objects/door, not the MoveProcessor
		if (GameObject* above = map_->view(door->pos_above())) {
			if (Player* player = dynamic_cast<Player*>(above)) {
				door_travelling_objs_.push_back({ player, player->pos_ });
			} else if (Car* car = dynamic_cast<Car*>(above->modifier())) {
				if (Player* player = dynamic_cast<Player*>(map_->view(car->pos_above()))) {
					if (player->state_ == RidingState::Riding) {
						door_travelling_objs_.push_back({ player, player->pos_ });
						door_travelling_objs_.push_back({ above, above->pos_ });
					}
				}
			}
		}
		if (door_travelling_objs_.empty()) {
			return;
		}
		door_state_ = DoorState::AwaitingEntry;
		entry_door_ = door;
	}
}

void MoveProcessor::try_door_entry() {
	if (playing_state_->can_use_door(entry_door_, door_travelling_objs_, &dest_room_)) {
		if (dest_room_->map() == map_) {
			door_state_ = DoorState::AwaitingIntExit;
		} else {
			door_state_ = DoorState::AwaitingExtExit;
		}
		for (auto& obj : door_travelling_objs_) {
			map_->take_real(obj.raw, delta_frame_);
			add_neighbors_to_fall_check(obj.raw);
		}
		frames_ = FALL_MOVEMENT_FRAMES;
		state_ = MoveStep::DoorMove;
	} else {
		entry_door_ = nullptr;
		door_travelling_objs_.clear();
		door_state_ = DoorState::None;
	}
}

void MoveProcessor::try_int_door_exit() {
	bool can_move = true;
	for (auto& obj : door_travelling_objs_) {
		if (map_->view(obj.dest)) {
			can_move = false;
			break;
		}
	}
	if (can_move) {
		for (auto& obj : door_travelling_objs_) {
			add_to_fall_check(obj.raw);
			obj.raw->abstract_put(obj.dest, delta_frame_);
			map_->put_real(obj.raw, delta_frame_);
		}
		frames_ = FALL_MOVEMENT_FRAMES;
		door_state_ = DoorState::Succeeded;
		state_ = MoveStep::PostDoorFallCheck;
	} else {
		frames_ = FALL_MOVEMENT_FRAMES;
		door_state_ = DoorState::AwaitingUnentry;
		state_ = MoveStep::DoorMove;
	}
}

void MoveProcessor::try_door_unentry() {
	bool can_move = true;
	for (auto& obj : door_travelling_objs_) {
		if (map_->view(obj.raw->pos_)) {
			can_move = false;
			break;
		}
	}
	if (can_move) {
		for (auto& obj : door_travelling_objs_) {
			add_to_fall_check(obj.raw);
			map_->put_real(obj.raw, delta_frame_);
		}
		frames_ = FALL_MOVEMENT_FRAMES;
		door_state_ = DoorState::Succeeded;
		state_ = MoveStep::PostDoorFallCheck;
	} else {
		std::cout << "TERRIBLE THINGS HAPPENED! (we're in the void!)" << std::endl;
		door_state_ = DoorState::Voided;
	}
}

void MoveProcessor::ext_door_exit() {
	delta_frame_->push(std::make_unique<RoomChangeDelta>(playing_state_, playing_state_->active_room()));
	playing_state_->activate_room(dest_room_);
	map_ = dest_room_->map();
	for (auto& obj : door_travelling_objs_) {
		add_to_fall_check(obj.raw);
		obj.raw->abstract_put(obj.dest, delta_frame_);
		map_->put_real(obj.raw, delta_frame_);
	}
	playing_state_->snap_camera_to_player();
	frames_ = FALL_MOVEMENT_FRAMES;
	door_state_ = DoorState::Succeeded;
	state_ = MoveStep::PostDoorFallCheck;
}

// NOTE: could be dangerous if repeated calls are made
// Either make sure this doesn't happen, or check for presence here.
void MoveProcessor::add_to_moving_blocks(GameObject* obj) {
	moving_blocks_.push_back(obj);
}

void MoveProcessor::add_to_fall_check(GameObject* obj) {
	fall_check_.push_back(obj);
}

void MoveProcessor::add_gate_transition(GateBody* gate_body, bool state) {
	if (animated_) {
		//gate_body->set_gate_transition_animation(state, this);
		//gate_transitions_.push_back(std::make_pair(gate_body, state));
	}
}

//TODO: fix this broken animation hack
void MoveProcessor::update_gate_transitions() {
/*	
	for (auto& p : gate_transitions_) {
		// A retracting object disappears at this point
		if (p.first->update_state_animation() && !p.second) {
			//map_->take_loud(p.first, delta_frame_);
		}
	}
	gate_transitions_.erase(std::remove_if(gate_transitions_.begin(), gate_transitions_.end(),
		[](auto& p) {return !p.first->state_animation(); }),
		gate_transitions_.end());
*/
}