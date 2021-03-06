#include "stdafx.h"
#include "gate.h"

#include "point.h"

#include "gatebody.h"
#include "mapfile.h"
#include "roommap.h"
#include "delta.h"

#include "moveprocessor.h"
#include "animationmanager.h"
#include "graphicsmanager.h"
#include "texture_constants.h"
#include "gameobjectarray.h"


Gate::Gate(GameObject* parent, GateBody* body, int color, int count, bool persistent, bool def, bool active, bool waiting) :
	Switchable(parent, count, persistent, def, active, waiting), color_{ color }, body_{ body } {}

Gate::~Gate() {}

void Gate::make_str(std::string& str) {
	str += "Gate";
	Switchable::make_str(str);
}

ModCode Gate::mod_code() {
	return ModCode::Gate;
}

void Gate::serialize(MapFileO& file) {
	bool holding_body = body_ && !state();
	file << color_ << count_ << persistent_ << default_ << active_ << waiting_ << holding_body;
	if (holding_body) {
		file << Point3_S16{ body_->pos_ - pos() };
	}
}

void Gate::serialize_with_ids(MapFileO& file) {
	bool holding_body = body_ && !state();
	file << color_ << count_ << persistent_ << default_ << active_ << waiting_ << holding_body;
	if (holding_body) {
		file << Point3_S16{ body_->pos_ - pos() };
		file.write_uint32(body_->id_);
	}
}

void Gate::deserialize(MapFileI& file, RoomMap* room_map, GameObject* parent) {
	unsigned char b[7];
	file.read(b, 7);
	auto gate = std::make_unique<Gate>(parent, nullptr, b[0], b[1], b[2], b[3], b[4], b[5]);
	// Is the body alive and retracted?
	if (b[6]) {
		Point3_S16 body_pos{};
		file >> body_pos;
		room_map->obj_array_.push_object(std::make_unique<GateBody>(gate.get(), Point3{ body_pos } + parent->pos_));
	}
	parent->set_modifier(std::move(gate));
}

void Gate::deserialize_with_ids(MapFileI& file, RoomMap* room_map, GameObject* parent) {
	unsigned char b[7];
	file.read(b, 7);
	auto gate = std::make_unique<Gate>(parent, nullptr, b[0], b[1], b[2], b[3], b[4], b[5]);
	// Is the body alive and retracted?
	if (b[6]) {
		Point3 body_pos = file.read_spoint3();
		auto body_unique = std::make_unique<GateBody>(gate.get(), Point3{ body_pos } + parent->pos_);
		auto id = file.read_uint32();
		body_unique->id_ = id;
		room_map->obj_array_.push_object(std::move(body_unique));
	}
	parent->set_modifier(std::move(gate));
}

GameObject* Gate::get_subordinate_object() {
	if (body_ && !state()) {
		return body_;
	}
	return nullptr;
}

void Gate::collect_special_links(std::vector<GameObject*>& to_check) {
	if (body_ && state()) {
		to_check.push_back(body_);
	}
}

bool Gate::can_set_state(bool state, RoomMap* map) {
	return body_ && (!state || (map->view(body_->pos_) == nullptr));
}

void Gate::apply_state_change(RoomMap* map, DeltaFrame* delta_frame, MoveProcessor* mp) {
	if (body_) {
		bool cur_state = state();
		if (cur_state && !body_->tangible_) {
			mp->push_rising_gate(this);
		} else if (!cur_state && body_->tangible_) {
			map->take_from_map(body_, true, true, delta_frame);
			mp->anims_->receive_signal(AnimationSignal::GateDown, parent_, delta_frame);
			mp->add_to_fall_check(parent_);
			GameObject* above = map->view(body_->pos_ + Point3{ 0,0,1 });
			if (above && above->gravitable_) {
				mp->add_to_fall_check(above);
			}
		}
	}
}

void Gate::raise_gate(RoomMap* map, DeltaFrame* delta_frame, MoveProcessor* mp) {
	map->put_in_map(body_, true, true, delta_frame);
	mp->anims_->receive_signal(AnimationSignal::GateUp, parent_, delta_frame);
}

bool Gate::update_animation(PlayingState*) {
	if (animation_time_ > 0) {
		--animation_time_;
		return false;
	} else {
		animation_state_ = GateAnimationState::None;
		return true;
	}
}

void Gate::start_raise_animation() {
	animation_state_ = GateAnimationState::Raise;
	animation_time_ = MAX_GATE_ANIMATION_FRAMES;
}

void Gate::start_lower_animation() {
	animation_state_ = GateAnimationState::Lower;
	animation_time_ = MAX_GATE_ANIMATION_FRAMES;
}

void Gate::map_callback(RoomMap* map, DeltaFrame* delta_frame, MoveProcessor* mp) {
	if (parent_->tangible_ && body_) {
		check_waiting(map, delta_frame, mp);
	}
}

void Gate::setup_on_put(RoomMap* map, DeltaFrame* delta_frame, bool real) {
	Switchable::setup_on_put(map, delta_frame, real);
	if (body_) {
		map->add_listener(this, body_->pos_);
		map->activate_listener_of(this);
	}
}

void Gate::cleanup_on_take(RoomMap* map, DeltaFrame* delta_frame, bool real) {
	Switchable::cleanup_on_take(map, delta_frame, real);
	if (body_) {
		map->remove_listener(this, body_->pos_);
	}
}

void Gate::destroy(MoveProcessor* mp, CauseOfDeath death) {
	mp->delta_frame_->push(std::make_unique<ModDestructionDelta>(this));
	if (body_) {
		body_->set_gate(nullptr);
	}
}

void Gate::undestroy() {
	if (body_) {
		body_->set_gate(this);
	}
}

// Consider making the Gate(base) visually different when the gate is active (only visible if the GateBody has been separated)
void Gate::draw(GraphicsManager* gfx, FPoint3 p) {
	BlockTexture tex;
	if (state()) {
		tex = BlockTexture::GateBaseExtended;
	} else {
		if (persistent_) {
			if (default_) {
				tex = BlockTexture::GateBasePersistentOn;
			} else {
				tex = BlockTexture::GateBasePersistentOff;
			}
		} else {
			tex = BlockTexture::GateBase;
		}
	}
	ModelInstancer& model = parent_->is_snake() ? gfx->top_diamond : gfx->top_cube;
	model.push_instance(glm::vec3(p.x, p.y, p.z + 0.5f), glm::vec3(0.7f, 0.7f, 0.1f), tex, color_);
	if (animation_state_ == GateAnimationState::Lower) {
		body_->draw(gfx);
	}
}

std::unique_ptr<ObjectModifier> Gate::duplicate(GameObject* parent, RoomMap* map, DeltaFrame* delta_frame) {
	auto dup = std::make_unique<Gate>(*this);
	dup->parent_ = parent;
	if (body_) {
		auto body_dup = std::make_unique<GateBody>(dup.get(), body_->pos_);
		dup->body_ = body_dup.get();
		// A GateBody can't be duplicated unless it's intangible
		map->push_to_object_array(std::move(body_dup), delta_frame);
	}
	dup->connect_to_signalers();
	return std::move(dup);
}
