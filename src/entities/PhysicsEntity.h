#pragma once
#ifndef ATMOS_PHYSICSENTITY_H
#define ATMOS_PHYSICSENTITY_H

#include "Entity.h"
#include "attributes/Physics.h"
#include "attributes/ModelInstance.h"
#include "core/storage.h"

struct PhysicsEntity : public Entity {
	void Init(const char* _name, Transform _transform, Model* _model, Collider* _collider, f32 _mass, b32 _static = false){
		type = EntityType_Physics;
		name = _name;
		transform = _transform;
		
		Assert(_model);
		AtmoAdmin->modelArr.add(ModelInstance(_model));
		model = AtmoAdmin->modelArr.last;
		model->attribute.entity = this;
		
		AtmoAdmin->physicsArr.add(Physics());
		physics = AtmoAdmin->physicsArr.last;
		physics->attribute.entity = this;
		physics->collider       = _collider;
		physics->position       = _transform.position;
		physics->rotation       = _transform.rotation;
		physics->scale          = _transform.scale;
		physics->mass           = _mass;
		physics->staticPosition = _static;
		physics->staticRotation = _static;
		
		id = AtmoAdmin->entities.count;
		AtmoAdmin->entities.add(this);
	}
};

#endif //ATMOS_PHYSICSENTITY_H