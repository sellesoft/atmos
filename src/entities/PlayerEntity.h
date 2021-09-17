#pragma once
#ifndef ATMOS_PLAYERENTITY_H
#define ATMOS_PLAYERENTITY_H

#include "Entity.h"
#include "attributes/Player.h"
#include "attributes/Physics.h"
#include "attributes/Movement.h"
#include "attributes/ModelInstance.h"
#include "core/storage.h"

struct PlayerEntity : public Entity {
	ModelInstance* model;
	Physics*       physics;
    
	void Init(const char* name, Mesh* mesh, Transform transform, f32 mass) {
		type = EntityType_Player;
		this->name = name;
		this->transform = transform;
        
        AtmoAdmin->modelArr.add(ModelInstance((mesh) ? mesh : Storage::NullMesh()));
		model = AtmoAdmin->modelArr.last; modelPtr = model;
		model->attribute.entity = this;
        
        AtmoAdmin->physicsArr.add(Physics());
        physics = AtmoAdmin->physicsArr.last; physicsPtr = physics;
		physics->attribute.entity = this;
        physics->collider   = new AABBCollider(model->mesh, mass);
        physics->position   = transform.position;
        physics->rotation   = transform.rotation;
        physics->scale      = transform.scale;
        physics->mass       = mass;
        physics->elasticity = 0;
	}
    
	void SendEvent(Event event) override {};
	void ReceiveEvent(Event event) override {};
};


#endif //ATMOS_PLAYERENTITY_H