#pragma once
#ifndef ATMOS_PLAYERENTITY_H
#define ATMOS_PLAYERENTITY_H

#include "Entity.h"

#include "attributes/Player.h"
#include "attributes/Physics.h"
#include "attributes/Movement.h"
#include "attributes/Collider.h"
#include "attributes/ModelInstance.h"
#include "core/storage.h"


struct PlayerEntity : public Entity {
	Player        player;
	Movement      movement;
	Physics       physics;
	Collider      collider;
	ModelInstance model;

	PlayerEntity(Mesh* mesh, Transform transform, f32 mass) {
		player   = Player();
		physics  = Physics(transform.position, transform.rotation, mass, 0);
		movement = Movement(&physics);
		model    = ModelInstance((mesh) ? mesh : Storage::NullMesh());
		collider = AABBCollider(model.mesh, mass);

		attributes.add({ &player, &physics, &movement, &model, &collider });
	}

};


#endif