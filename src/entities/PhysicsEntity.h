#pragma once
#ifndef ATMOS_PHYSICSENTITY_H
#define ATMOS_PHYSICSENTITY_H

#include "Entity.h"

#include "attributes/Physics.h"
#include "attributes/Collider.h"
#include "attributes/ModelInstance.h"
#include "core/storage.h"

struct PhysicsEntity : public Entity {
	Physics       physics;
	Collider      collider;
	ModelInstance model;

	void Init(const char* name, Mesh* mesh, Transform transform, f32 mass, bool static_pos = 0, bool static_rot = 0) {
		type = EntityType_NonStaticMesh;
		this->name = name;
		this->transform = transform;

		physics  = Physics(transform.position, transform.rotation, mass, 0); physicsPtr  = &physics;
		model    = ModelInstance((mesh) ? mesh : Storage::NullMesh());       modelPtr    = &model;
		collider = AABBCollider(model.mesh, mass);                           colliderPtr = &collider;
		
		if (static_pos) { physics.staticPosition = 1;  type = EntityType_StaticMesh; }
		if (static_rot) physics.staticRotation = 1;
	}

	void SendEvent(Event event) override {};
	void ReceiveEvent(Event event) override {};

};


#endif