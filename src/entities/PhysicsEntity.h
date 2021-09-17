#pragma once
#ifndef ATMOS_PHYSICSENTITY_H
#define ATMOS_PHYSICSENTITY_H

#include "Entity.h"

#include "attributes/Physics.h"
#include "attributes/ModelInstance.h"
#include "core/storage.h"

struct PhysicsEntity : public Entity {
	ModelInstance* model;
	Physics*       physics;
    
	void Init(const char* name, Model* _model, Transform transform, f32 mass, bool static_pos = 0, bool static_rot = 0) {
		type = (static_pos && static_rot) ? EntityType_StaticMesh : EntityType_NonStaticMesh;
		this->name = name;
		this->transform = transform;
        
		AtmoAdmin->modelArr.add(ModelInstance((_model) ? _model : Storage::NullModel()));
		model = AtmoAdmin->modelArr.last; modelPtr = model;
        model->attribute.entity = this;
        
        AtmoAdmin->physicsArr.add(Physics());
        physics = AtmoAdmin->physicsArr.last; physicsPtr = physics;
		physics->attribute.entity = this;
        physics->collider       = new AABBCollider(_model->mesh, mass);
        physics->position       = transform.position;
        physics->rotation       = transform.rotation;
        physics->scale          = transform.scale;
        physics->mass           = mass;
        physics->staticPosition = static_pos;
        physics->staticRotation = static_rot;
	}
    
	void SendEvent(Event event) override {};
	void ReceiveEvent(Event event) override {};
};

#endif //ATMOS_PHYSICSENTITY_H