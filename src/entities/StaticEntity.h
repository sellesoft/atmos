#pragma once
#ifndef ATMOS_PHYSICSENTITY_H
#define ATMOS_PHYSICSENTITY_H

#include "Entity.h"

#include "attributes/Collider.h"
#include "attributes/ModelInstance.h"
#include "core/storage.h"

//mesh that the player should never interact with but can see
struct SceneryEntity : public Entity {
	ModelInstance model;

	void Init(const char* name, Mesh* mesh, Transform transform, f32 mass) {
		type = EntityType_SceneryMesh;
		this->name = name;
		this->transform = transform;

		model = ModelInstance((mesh) ? mesh : Storage::NullMesh()); modelPtr = &model;
	}

	void SendEvent(Event event) override {};
	void ReceiveEvent(Event event) override {};
};


#endif