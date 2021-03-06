#include "stdafx.h"
#include "objecttab.h"

#include "color_constants.h"

#include "editorstate.h"
#include "room.h"
#include "roommap.h"

#include "gameobject.h"
#include "player.h"
#include "pushblock.h"
#include "snakeblock.h"
#include "wall.h"
#include "gatebody.h"
#include "gate.h"

ObjectTab::ObjectTab(EditorState* editor) :
	EditorTab(editor) {}

ObjectTab::~ObjectTab() {}

// Current object type
static ObjCode obj_code = ObjCode::Wall;

// Model objects that new objects are created from
static PushBlock model_pb{ Point3{0,0,0}, GREEN, true, true, Sticky::None };
static SnakeBlock model_sb{ Point3{0,0,0}, PURPLE, true, true, 2, true };
static ArtWall model_artwall{ Point3{0,0,0}, 0 };
static Player model_player{ Point3{0,0,0}, PlayerState::Free };


// Object Inspection
static GameObject* selected_obj = nullptr;
static Point3 selected_pos = { -1,-1,-1 };

void ObjectTab::init() {
	selected_obj = nullptr;
}

void ObjectTab::main_loop(EditorRoom* eroom) {
	ImGui::Text("The Object Tab");
	ImGui::Separator();

	if (!eroom) {
		ImGui::Text("No room loaded.");
		return;
	}

	ImGui::Checkbox("Inspect Mode##OBJECT_inspect", &inspect_mode_);
	ImGui::Separator();

	object_tab_options(eroom);
}

void ObjectTab::object_type_choice(ObjCode* obj_code_ptr) {
	ImGui::RadioButton("PushBlock##OBJECT_object", obj_code_ptr, ObjCode::PushBlock);
	ImGui::RadioButton("SnakeBlock##OBJECT_object", obj_code_ptr, ObjCode::SnakeBlock);
	ImGui::RadioButton("Wall##OBJECT_object", obj_code_ptr, ObjCode::Wall);
	ImGui::RadioButton("ArtWall##OBJECT_object", obj_code_ptr, ObjCode::ArtWall);
	ImGui::RadioButton("Player##OBJECT_object", obj_code_ptr, ObjCode::Player);
}

void ObjectTab::object_tab_options(EditorRoom* eroom) {
	RoomMap* map = eroom->map();
	GameObject* obj = nullptr;
	if (inspect_mode_) {
		if (selected_obj) {
			obj = selected_obj;
			ImGui::Text("Current selected object position: (%d,%d,%d)", selected_pos.x, selected_pos.y, selected_pos.z);
		} else {
			ImGui::Text("No object selected!");
			return;
		}
	} else {
		object_type_choice(&obj_code);
	}
	ImGui::Separator();
	switch (obj ? obj->obj_code() : obj_code) {
	case ObjCode::PushBlock:
	{
		ImGui::Text("PushBlock");
		PushBlock* pb = obj ? static_cast<PushBlock*>(obj) : &model_pb;
		ImGui::Checkbox("Pushable?##PB_modify_push", &pb->pushable_);
		ImGui::Checkbox("Gravitable?##PB_modify_grav", &pb->gravitable_);
		ImGui::InputInt("Color##PB_modify_COLOR", &pb->color_);
		color_button(pb->color_);
		ImGui::Text("Stickiness");
		ImGui::RadioButton("NonStick##PB_modify_sticky", &pb->sticky_, Sticky::None);
		ImGui::RadioButton("Weakly Sticky##PB_modify_sticky", &pb->sticky_, Sticky::WeakBlock);
		ImGui::RadioButton("Strongly Sticky##PB_modify_sticky", &pb->sticky_, Sticky::StrongBlock);
		ImGui::RadioButton("SemiWeak Sticky##PB_modify_sticky", &pb->sticky_, Sticky::SemiBlock);
		break;
	}
	case ObjCode::SnakeBlock:
	{
		ImGui::Text("SnakeBlock");
		SnakeBlock* sb = obj ? static_cast<SnakeBlock*>(obj) : &model_sb;
		ImGui::Checkbox("Pushable?##SB_modify_push", &sb->pushable_);
		ImGui::Checkbox("Gravitable?##SB_modify_grav", &sb->gravitable_);
		int prev_color = sb->color_;
		ImGui::InputInt("Color##SB_modify_COLOR", &sb->color_);
		color_button(sb->color_);
		if (obj && sb->color_ != prev_color) {
			sb->remove_wrong_color_links(nullptr);
		}
		ImGui::Text("Number of Ends");
		ImGui::RadioButton("One Ended##SB_modify_snake_ends", &sb->ends_, 1);
		ImGui::RadioButton("Two Ended##SB_modify_snake_ends", &sb->ends_, 2);
		if (sb->links_.size() > sb->ends_) {
			for (auto* link : sb->links_) {
				sb->remove_link_quiet(link);
			}
		}
		ImGui::Checkbox("Weak?##SB_modify_weak", &sb->weak_);
		break;
	}
	case ObjCode::GateBody:
	{
		ImGui::Text("GateBody");
		auto* gate_body = static_cast<GateBody*>(obj);
		if (auto* gate = gate_body->gate_) {
			Point3 p_pos = gate->pos();
			ImGui::Text("See parent Gate at (%d,%d,%d)", p_pos.x, p_pos.y, p_pos.z);
		}
		break;
	}
	case ObjCode::Player:
	{
		ImGui::Text("Player");
		if (selected_obj == map->view(eroom->start_pos)) {
			ImGui::Text("The Default Startpoint Player");
			return;
		}
		Player* player = obj ? static_cast<Player*>(obj) : &model_player;
		ImGui::Text("Player State");
		ImGui::RadioButton("Free##PLAYER_modify_state", &player->state_, PlayerState::Free);
		ImGui::RadioButton("Bound##PLAYER_modify_state", &player->state_, PlayerState::Bound);
		// Allowing the user to set "Riding" has annoying consequences
		//ImGui::RadioButton("Riding##PLAYER_modify_state", &player->state_, PlayerState::RidingNormal);
		break;
	}
	case ObjCode::Wall:
		ImGui::Text("Wall");
		break;
	case ObjCode::ArtWall:
	{
		ImGui::Text("ArtWall");
		ArtWall* artwall = obj ? static_cast<ArtWall*>(obj) : &model_artwall;
		ImGui::InputInt("Wall Flavor##ARTWALL_flavor", &artwall->flavor_);
		break;
	}
	}
	if (inspect_mode_ && selected_obj) {
		static ObjCode transmute_obj_code;
		ImGui::Separator();
		ImGui::Text("Transmute Object");
		object_type_choice(&transmute_obj_code);
		if (ImGui::Button("Transmute##OBJECT")) {
			if (auto new_obj = create_from_model(transmute_obj_code, selected_obj)) {
				if (selected_obj->id_ == GENERIC_WALL_ID) {
					map->clear(selected_pos);
				} else {
					map->take_from_map(selected_obj, true, false, nullptr);
					map->remove_from_object_array(selected_obj);
				}
				selected_obj = new_obj.get();
				map->create_in_map(std::move(new_obj), false, nullptr);
			}
			// If the new object was a generic Wall, we don't have a new pointer
			else if (transmute_obj_code == ObjCode::Wall) {
				// If the old object was also a generic Wall, do nothing
				if (selected_obj->id_ != GENERIC_WALL_ID) {
					map->take_from_map(selected_obj, true, false, nullptr);
					map->remove_from_object_array(selected_obj);
					map->create_wall(selected_pos);
					selected_obj = map->view(selected_pos);
				}
			}
		}
	}
}

std::unique_ptr<GameObject> ObjectTab::create_from_model(ObjCode obj_code, GameObject* prev) {
	std::unique_ptr<GameObject> obj{};
	switch (obj_code) {
	case ObjCode::Wall:
		if (!(prev && prev->modifier_)) {
			return nullptr;
		}
		obj = std::make_unique<Wall>(selected_pos);
		break;
	case ObjCode::PushBlock:
		obj = std::make_unique<PushBlock>(model_pb);
		break;
	case ObjCode::SnakeBlock:
		obj = std::make_unique<SnakeBlock>(model_sb);
		break;
	case ObjCode::ArtWall:
		obj = std::make_unique<ArtWall>(model_artwall);
		break;
	case ObjCode::Player:
		obj = std::make_unique<Player>(model_player);
		break;
	default:
		return nullptr;
	}
	if (prev) {
		if (auto* mod = prev->modifier()) {
			// The old modifier can't live on the new object; destroy it
			if (!mod->valid_parent(obj.get())) {
				prev->modifier_->cleanup_on_editor_destruction(editor_->global_.get());
			// Move the old modifier to the new object
			} else {
				obj->modifier_ = std::move(prev->modifier_);
				obj->modifier_->parent_ = obj.get();
			}
			prev->modifier_.reset(nullptr);
		}
	}
	obj->pos_ = selected_pos;
	return std::move(obj);
}

bool ObjectTab::handle_keyboard_input() {
	if (EditorTab::handle_keyboard_input()) {
		return true;
	}
	if (key_pressed(GLFW_KEY_LEFT_SHIFT)) {
		for (int i = 1; i <= 3; ++i) {
			if (key_pressed(GLFW_KEY_0 + i)) {
				obj_code = static_cast<ObjCode>(i);
			}
		}
	}
	return false;
}

void ObjectTab::handle_left_click(EditorRoom* eroom, Point3 pos) {
	RoomMap* map = eroom->map();
	if (!map->valid(pos)) {
		selected_obj = nullptr;
		return;
		// Even in create mode, let the user select an already-created object
	}
	selected_pos = pos;
	if (inspect_mode_ || map->view(pos)) {
		selected_obj = map->view(pos);
		return;
	}
	if (std::unique_ptr<GameObject> obj = create_from_model(obj_code, nullptr)) {
		selected_obj = obj.get();
		map->create_in_map(std::move(obj), false, nullptr);
	} else if (obj_code == ObjCode::Wall) {
		map->create_wall(pos);
		selected_obj = map->view(pos);
	}
}

void ObjectTab::handle_right_click(EditorRoom* eroom, Point3 pos) {
	// Don't allow deletion in inspect mode
	if (inspect_mode_) {
		return;
	}
	if (kill_object(pos, eroom->map(), eroom->start_pos)) {
		selected_obj = nullptr;
	}
}