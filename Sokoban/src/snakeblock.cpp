#include "stdafx.h"
#include "snakeblock.h"
#include "graphicsmanager.h"
#include "texture_constants.h"

#include "mapfile.h"
#include "component.h"
#include "delta.h"
#include "roommap.h"

#include "objectmodifier.h"
#include "autoblock.h"
#include "car.h"


SnakeBlock::SnakeBlock(Point3 pos, int color, bool pushable, bool gravitable, int ends, bool weak) :
	ColoredBlock(pos, color, pushable, gravitable), ends_{ ends }, weak_{ weak }  {}

SnakeBlock::~SnakeBlock() {}

std::string SnakeBlock::name() {
	return "SnakeBlock";
}

ObjCode SnakeBlock::obj_code() {
	return ObjCode::SnakeBlock;
}

void SnakeBlock::serialize(MapFileO& file) {
	file << color_ << pushable_ << gravitable_ << ends_ << weak_;
}

std::unique_ptr<GameObject> SnakeBlock::deserialize(MapFileI& file) {
	Point3 pos{ file.read_point3() };
	unsigned char b[5];
	file.read(b, 5);
	return std::make_unique<SnakeBlock>(pos, b[0], b[1], b[2], b[3], b[4]);
}

bool SnakeBlock::relation_check() {
	return true;
}

void SnakeBlock::relation_serialize(MapFileO& file) {
	int link_encode = 0;
	for (auto& link : links_) {
		Point3 q = link->pos_;
		// Snake links are always adjacent, and we only bother to
		// record links to the Right or Down
		if (q.x > pos_.x) {
			++link_encode;
		} else if (q.y > pos_.y) {
			link_encode += 2;
		}
	}
	if (link_encode) {
		file << MapCode::SnakeLink;
		file << pos_;
		file << link_encode;
	}
}

bool SnakeBlock::is_snake() {
	return true;
}

Sticky SnakeBlock::sticky() {
	return weak_ ? Sticky::WeakSnake : Sticky::StrongSnake;
}

void SnakeBlock::collect_sticky_links(RoomMap* map, Sticky sticky_level, std::vector<GameObject*>& links) {
	Sticky sticky_condition = sticky() & sticky_level;
	for (SnakeBlock* link : links_) {
		if ((sticky_condition & link->sticky()) != Sticky::None) {
			links.push_back(link);
		}
	}
}

void SnakeBlock::reset_internal_state() {
	distance_ = 0;
	target_ = nullptr;
	dragged_ = false;
}

void SnakeBlock::collect_dragged_snake_links(RoomMap* map, Point3 dir, std::vector<GameObject*>& links, bool strong) {
	// If this block is weak, it can't drag strongly
	if (strong && weak_) {
		return;
	}
	// Were we pushed by the object behind us? If so, drag all links
	// NOTE: this does no harm even if obj is a link
	if (GameObject* obj = map->view(pos_ - dir)) {
		if (obj->comp_) {
			for (SnakeBlock* link : links_) {
				link->conditional_drag(links, strong);
			}
			return;
		}
	}
	// If there aren't 2 links, there's no need to drag anything
	if (links_.size() < 2) {
		return;
	}
	for (int i = 0; i < 2; ++i) {
		Point3 link_pos{ links_[i]->pos_ };
		// If there's a link behind us, drag the other
		if (link_pos + dir == pos_) {
			links_[1 - i]->conditional_drag(links, strong);
			return;
		}
		// If there's a link in front of us, don't drag anything
		if (link_pos == pos_ + dir) {
			return;
		}
	}
	// At this point, we have 2 links, both of which are to the side
	for (SnakeBlock* link : links_) {
		link->conditional_drag(links, strong);
	}
}

void SnakeBlock::conditional_drag(std::vector<GameObject*>& links, bool strong) {
	if (!(strong && weak_) && !comp_) {
		dragged_ = true;
		links.push_back(this);
	}
}

bool SnakeBlock::moving_push_comp() {
	if (PushComponent* comp = push_comp()) {
		return comp->moving_;
	} else {
		return false;
	}
}

void SnakeBlock::draw(GraphicsManager* gfx) {
	FPoint3 p{ real_pos() };
	BlockTexture tex;
	if (weak_) {
		if (ends_ == 1) {
			tex = BlockTexture::BrokenEdges;
		} else {
			tex = BlockTexture::Edges;
		}
	} else {
		if (ends_ == 1) {
			tex = BlockTexture::Corners;
		} else {
			tex = BlockTexture::LightEdges;
		}
	}
	if (modifier_) {
		tex = tex | modifier_->texture();
	}
	gfx->diamond.push_instance(glm::vec3(p.x, p.y, p.z), glm::vec3(1.0f, 1.0f, 1.0f), tex, color_);
	draw_force_indicators(gfx->diamond, p, 1.1f);
	for (auto link : links_) {
		FPoint3 q = link->real_pos();
		FPoint3 d{ q.x - p.x, q.y - p.y, 0 };
		gfx->cube.push_instance(glm::vec3(p.x + 0.2f*d.x, p.y + 0.2f*d.y, p.z + 0.5f),
			glm::vec3(0.1f + 0.2f*abs(d.x), 0.1f + 0.2f*abs(d.y), 0.2), BlockTexture::Blank, BLACK);
	}
	if (modifier_) {
		modifier()->draw(gfx, p);
	}
}


bool SnakeBlock::in_links(SnakeBlock* sb) {
	return std::find(links_.begin(), links_.end(), sb) != links_.end();
}

void SnakeBlock::add_link(SnakeBlock* sb, DeltaFrame* delta_frame) {
	add_link_quiet(sb);
	delta_frame->push(std::make_unique<AddLinkDelta>(this, sb));
}

void SnakeBlock::add_link_quiet(SnakeBlock* sb) {
	links_.push_back(sb);
	sb->links_.push_back(this);
}

void SnakeBlock::add_link_one_way(SnakeBlock* sb) {
	links_.push_back(sb);
}

void SnakeBlock::remove_link(SnakeBlock* sb, DeltaFrame* delta_frame) {
	remove_link_quiet(sb);
	delta_frame->push(std::make_unique<RemoveLinkDelta>(this, sb));
}

void SnakeBlock::remove_link_quiet(SnakeBlock* sb) { 
	links_.erase(std::find(links_.begin(), links_.end(), sb));
	sb->links_.erase(std::find(sb->links_.begin(), sb->links_.end(), this));
}

void SnakeBlock::remove_link_one_way(SnakeBlock* sb) {
	links_.erase(std::find(links_.begin(), links_.end(), sb));
}

void SnakeBlock::remove_link_one_way_undoable(SnakeBlock* sb, DeltaFrame* delta_frame) {
	links_.erase(std::find(links_.begin(), links_.end(), sb));
	delta_frame->push(std::make_unique<RemoveLinkOneWayDelta>(this, sb));
}

bool SnakeBlock::can_link(SnakeBlock* snake) {
	return available() && snake->available() &&
		(color() == snake->color()) && (pos_.z == snake->pos_.z) &&
		(abs(pos_.x - snake->pos_.x) + abs(pos_.y - snake->pos_.y) == 1) &&
		!in_links(snake);
}

void SnakeBlock::check_add_local_links(RoomMap* map, DeltaFrame* delta_frame) {
	if (!available() || confused(map)) {
		return;
	}
	for (auto& d : H_DIRECTIONS) {
		auto snake = dynamic_cast<SnakeBlock*>(map->view(shifted_pos(d)));
		if (snake && color() == snake->color() && snake->available() && !in_links(snake) && !snake->confused(map)) {
			add_link(snake, delta_frame);
		}
	}
}

void SnakeBlock::collect_maybe_confused_neighbors(RoomMap* map, std::set<SnakeBlock*>& check) {
	if (available()) {
		for (Point3 d : H_DIRECTIONS) {
			auto snake = dynamic_cast<SnakeBlock*>(map->view(shifted_pos(d)));
			// TODO: Make sure these conditions are reasonable
			if (snake && (color_ == snake->color_) && snake->available()) {
				check.insert(snake);
			}
		}
	}
}

void SnakeBlock::break_blocked_links_horizontal(std::vector<GameObject*>& fall_check, RoomMap* map, DeltaFrame* delta_frame, Point3 dir) {
	bool break_blocked = false;
	// If there's something moving behind us, break all blocked links
	if (GameObject* obj = map->view(pos_ - dir)) {
		if (obj->comp_) {
			break_blocked = true;
		}
	}
	// If we have two links, and neither is in front, break all blocked links
	if (!break_blocked && links_.size() == 2) {
		break_blocked = true;
		for (auto* link : links_) {
			if (link->pos_ == pos_ + dir) {
				break_blocked = false;
				break;
			}
		}
	}
	if (break_blocked){
		break_blocked_links(fall_check, delta_frame);
	}
}

void SnakeBlock::break_blocked_links(std::vector<GameObject*>& fall_check, DeltaFrame* delta_frame) {
	auto links_copy = links_;
	for (SnakeBlock* link : links_copy) {
		if (PushComponent* comp = link->push_comp()) {
			// If both blocks are strong, don't break the link!!
			if (comp->blocked_ && (weak_ || link->weak_)) {
				remove_link(link, delta_frame);
				fall_check.push_back(link);
			}
		}
	}
}

void SnakeBlock::remove_wrong_color_links(DeltaFrame* delta_frame) {
	auto links_copy = links_;
	for (auto link : links_copy) {
		if (color_ != link->color_) {
			if (delta_frame) {
				remove_link(link, delta_frame);
			} else {
				remove_link_quiet(link);
			}
		}
	}
}

// Call when a group of snake blocks becomes intangible, in particular for door movement
void SnakeBlock::break_tangible_links(DeltaFrame* delta_frame, std::vector<GameObject*>& fall_check) {
	auto links_copy = links_;
	for (auto link : links_copy) {
		if (link->tangible_) {
			remove_link_one_way_undoable(link, delta_frame);
			fall_check.push_back(link);
		}
	}
}

bool SnakeBlock::available() {
	return links_.size() < ends_;
}

bool SnakeBlock::confused(RoomMap* map) {
	int available_count = 0;
	for (auto& d : H_DIRECTIONS) {
		auto snake = dynamic_cast<SnakeBlock*>(map->view(shifted_pos(d)));
		if (snake && color_ == snake->color_ && (snake->available() || in_links(snake))) {
			++available_count;
		}
	}
	return available_count > ends_;
}

void SnakeBlock::cleanup_on_take(RoomMap* map, bool real) {
	reset_internal_state();
	if (real) {
		for (SnakeBlock* link : links_) {
			link->remove_link_one_way(this);
		}
	}
	if (modifier_) {
		modifier_->cleanup_on_take(map, real);
	}
}

void SnakeBlock::setup_on_put(RoomMap* map, bool real) {
	if (real) {
		for (SnakeBlock* link : links_) {
			link->add_link_one_way(this);
		}
	}
	if (modifier_) {
		modifier_->setup_on_put(map, real);
	}
}

std::unique_ptr<SnakeBlock> SnakeBlock::make_split_copy(RoomMap* map, DeltaFrame* delta_frame) {
	auto split = std::make_unique<SnakeBlock>(pos_, color_, pushable_, gravitable_, 1, weak_);
	if (modifier_) {
		split->set_modifier(modifier_->duplicate(split.get(), map, delta_frame));
	}
	return std::move(split);
}


SnakePuller::SnakePuller(MoveProcessor* mp, RoomMap* map, DeltaFrame* delta_frame,
	std::vector<GameObject*>& moving_blocks,
	std::set<SnakeBlock*>& link_add_check,
	std::vector<GameObject*>& fall_check) :
	move_processor_{ mp }, map_{ map }, delta_frame_{ delta_frame },
	moving_blocks_{ moving_blocks }, link_add_check_{ link_add_check }, fall_check_{ fall_check } {}

SnakePuller::~SnakePuller() {}

void SnakePuller::prepare_pull(SnakeBlock* cur) {
	SnakeBlock* prev{};
	// A moving snake can have at most one link which isn't moving already
	// That's the one we want to pull.
	for (SnakeBlock* link : cur->links_) {
		if (!link->moving_push_comp()) {
			prev = cur;
			cur = link;
			break;
		}
	}
	// This block didn't have anything to pull!
	if (!prev) {
		return;
	}
	prev->distance_ = 1;
	cur->target_ = prev;
	// At the beginning of this loop, it's guaranteed that cur->comp_ == nullptr
	while (true) {
		// We've reached a block for the second time (i.e. from the other direction)
		if (cur->distance_ > 0) {
			// The chain was odd length; split the middle block!
			if (cur->distance_ == prev->distance_ + 1) {
				// TODO: Make sure this is *really* the condition we want
				// For now, a snake which is linked to anything else
				//(at level Sticky::AllStick) will not be split.
				std::vector<GameObject*> sticky_comp{};
				cur->collect_special_links(map_, sticky_comp);
				cur->reset_internal_state();
				if (ObjectModifier* mod = cur->modifier()) {
					mod->collect_special_links(map_, sticky_comp);
				}
				// The split succeeded
				if (sticky_comp.empty()) {
					std::vector<SnakeBlock*> links = cur->links_;
					// Listeners will get alerted during snake pulling anyway
					map_->take_from_map(cur, true, false, delta_frame_);
					for (SnakeBlock* link : links) {
						auto split_copy_unique = cur->make_split_copy(map_, delta_frame_);
						SnakeBlock* split_copy = split_copy_unique.get();
						map_->create_in_map(std::move(split_copy_unique), false, delta_frame_);
						split_copy->add_link(link, delta_frame_);
						split_copy->target_ = link;
						snakes_to_pull_.push_back(split_copy);
					}
					cur->destroy(move_processor_, CauseOfDeath::Split, false);
				// The middle block couldn't be split; split around instead
				} else {
					std::vector<SnakeBlock*> links = cur->links_;
					link_add_check_.insert(cur);
					for (SnakeBlock* link : links) {
						cur->remove_link(link, delta_frame_);
						link_add_check_.insert(link);
						snakes_to_pull_.push_back(link);
					}
				}
				return;
				// The chain was even length; cut!
			} else if (cur->distance_ == prev->distance_) {
				link_add_check_.insert(cur);
				link_add_check_.insert(prev);
				cur->remove_link(prev, delta_frame_);
				snakes_to_pull_.push_back(cur);
				snakes_to_pull_.push_back(prev);
				return;
			}
		}
		cur->target_ = prev;
		// If cur is the end of the snake, pull it
		if (cur->links_.size() == 1) {
			link_add_check_.insert(cur);
			cur->collect_maybe_confused_neighbors(map_, link_add_check_);
			snakes_to_pull_.push_back(cur);
			return;
		}
		// Progress down the snake
		for (SnakeBlock* link : cur->links_) {
			if (link != prev) {
				cur->distance_ = prev->distance_ + 1;
				prev = cur;
				cur = link;
				break;
			}
		}
		if (cur->moving_push_comp()) {
			return;
		}
	}
}

void SnakePuller::perform_pulls() {
	for (SnakeBlock* cur : snakes_to_pull_) {
		while (SnakeBlock* next = cur->target_) {
			cur->reset_internal_state();
			moving_blocks_.push_back(cur);
			Point3 dir = next->pos_ - cur->pos_;
			cur->set_linear_animation(dir);
			map_->shift(cur, dir, true, delta_frame_);
			cur = next;
		}
		cur->reset_internal_state();
	}
}
