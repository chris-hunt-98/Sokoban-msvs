#include "stdafx.h"
#include "gameobjectarray.h"

#include "wall.h"
#include "objectmodifier.h"
#include "animation.h"

GameObjectArray::GameObjectArray(): array_ {} {
    array_.push_back(nullptr);
    array_.push_back(std::make_unique<Wall>());
    array_[1]->id_ = 1;
}

GameObjectArray::~GameObjectArray() {}

void GameObjectArray::push_object(std::unique_ptr<GameObject> obj) {
    obj->id_ = (int)array_.size();
    array_.push_back(std::move(obj));
}

GameObject* GameObjectArray::operator[](unsigned int id) const {
    return array_[id].get();
}

GameObject* GameObjectArray::safe_get(unsigned int id) const {
    if (id >= array_.size()) {
        return nullptr;
    } else {
        return array_[id].get();
    }
}

// TODO: use a minheap of "deleted IDs" to potentially reuse object IDs.
void GameObjectArray::destroy(GameObject* obj) {
    array_[obj->id_].reset(nullptr);
}
