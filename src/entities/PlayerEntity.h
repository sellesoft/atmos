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

	void Init(const char* name, Mesh* mesh, Transform transform, f32 mass) {
		type = EntityType_Player;
		this->name = name;
		this->transform = transform;

		player   = Player();                                                 playerPtr   = &player;
		physics  = Physics(transform.position, transform.rotation, mass, 0); physicsPtr  = &physics;
		movement = Movement(&physics);                                       movementPtr = &movement;
		model    = ModelInstance((mesh) ? mesh : Storage::NullMesh());       modelPtr    = &model;
		collider = AABBCollider(model.mesh, mass);                           colliderPtr = &collider;
		
		//this sucks do it differently later
		player.  attribute.entity = this;
		physics. attribute.entity = this;
		movement.attribute.entity = this;
		model.   attribute.entity = this;
		collider.attribute.entity = this;


	}

	void SendEvent(Event event) override {};
	void ReceiveEvent(Event event) override {};

};


#endif