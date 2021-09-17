#pragma once
#ifndef ATMOS_STATICENTITY_H
#define ATMOS_STATICENTITY_H

#include "Entity.h"

#include "attributes/Collider.h"
#include "attributes/ModelInstance.h"
#include "core/storage.h"

//mesh that the player should never interact with but can see
struct SceneryEntity : 
	public Entity, 
	public ModelInstance{
	//ModelInstance model;

	void Init(const char* name, Mesh* mesh, Transform transform, f32 mass) {
		Entity::type      = EntityType_SceneryMesh;
		Entity::name      = name;
		Entity::transform = transform;

		ModelInstance::ModelInstance((mesh) ? mesh : Storage::NullMesh());
	}

	void SendEvent(Event event) override {};
	void ReceiveEvent(Event event) override {};
};


#endif