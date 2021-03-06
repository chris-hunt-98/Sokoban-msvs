#include "stdafx.h"
#include "pushblock.h"

#include "roommap.h"
#include "mapfile.h"
#include "graphicsmanager.h"
#include "texture_constants.h"

#include "objectmodifier.h"
#include "autoblock.h"
#include "car.h"

PushBlock::PushBlock(Point3 pos, int color, bool pushable, bool gravitable, Sticky sticky):
ColoredBlock(pos, color, pushable, gravitable), sticky_ {sticky} {}

PushBlock::~PushBlock() {}

std::string PushBlock::name() {
    return "PushBlock";
}

ObjCode PushBlock::obj_code() {
    return ObjCode::PushBlock;
}

void PushBlock::serialize(MapFileO& file) {
    file << color_ << pushable_ << gravitable_ << sticky_;
}

std::unique_ptr<GameObject> PushBlock::deserialize(MapFileI& file, Point3 pos) {
    unsigned char b[4];
    file.read(b, 4);
    return std::make_unique<PushBlock>(pos, b[0], b[1], b[2], static_cast<Sticky>(b[3]));
}

std::unique_ptr<GameObject> PushBlock::duplicate(RoomMap* room_map, DeltaFrame* delta_frame) {
	auto dup = std::make_unique<PushBlock>(*this);
	if (modifier_) {
		dup->set_modifier(modifier_->duplicate(dup.get(), room_map, delta_frame));
	}
	return std::move(dup);
}

void PushBlock::collect_sticky_links(RoomMap* map, Sticky sticky_level, std::vector<GameObject*>& links) {
    Sticky sticky_condition = sticky_ & sticky_level;
    if (sticky_condition != Sticky::None) {
        for (Point3 d : DIRECTIONS) {
            PushBlock* adj = dynamic_cast<PushBlock*>(map->view(pos_ + d));
            if (adj && adj->color_ == color_ && ((adj->sticky_ & sticky_condition) != Sticky::None)) {
                links.push_back(adj);
            }
        }
    }
}

bool PushBlock::has_sticky_neighbor(RoomMap* map) {
	for (Point3 d : H_DIRECTIONS) {
		if (PushBlock* adj = dynamic_cast<PushBlock*>(map->view(pos_ + d))) {
			if ((adj->color_ == color_) && static_cast<bool>(adj->sticky() & sticky())) {
				return true;
			}
		}
	}
	return false;
}

Sticky PushBlock::sticky() {
    return sticky_;
}

BlockTexture PushBlock::get_block_texture() {
	BlockTexture tex{ BlockTexture::Blank };
	switch (sticky_) {
	case Sticky::None:
		tex = BlockTexture::SolidEdgesDark;
		break;
	case Sticky::WeakBlock:
		tex = BlockTexture::WeakSticky;
		break;
	case Sticky::StrongBlock:
		tex = BlockTexture::SolidEdgesLight;
		break;
	case Sticky::SemiBlock:
		tex = BlockTexture::SemiweakSticky;
		break;
	}
	if (modifier_) {
		tex = tex | modifier_->texture();
	}
	return tex;
}

void PushBlock::draw(GraphicsManager* gfx) {
	FPoint3 p{ real_pos() };
	gfx->cube.push_instance(glm::vec3(p), glm::vec3(1.0f), get_block_texture(), color_);
    draw_force_indicators(gfx->cube, p, 1.1f);
    if (modifier_) {
        modifier()->draw(gfx, p);
    }
}

void PushBlock::draw_squished(GraphicsManager* gfx, FPoint3 p, float scale) {
	gfx->cube.push_instance(glm::vec3(p), glm::vec3(scale, scale, 1.0f), get_block_texture(), color_);
	draw_force_indicators(gfx->cube, p, scale * 1.1f);
	if (auto* car = dynamic_cast<Car*>(modifier_.get())) {
		car->draw_squished(gfx, p, scale);
	}
}