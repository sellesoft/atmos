#pragma once
#ifndef ATMOS_DOORENTITY_H
#define ATMOS_DOORENTITY_H

#include "Entity.h"
#include "attributes/Physics.h"
#include "attributes/ModelInstance.h"
#include "attributes/InterpTransform.h"
#include "core/storage.h"

struct DoorEntity : public Entity{
	void Init(const char* _name, Collider _collider, Model* _model, Transform _start, Transform _end, f32 _duration){
		type = EntityType_Door;
		name = _name;
		transform = _start;
		
		AtmoAdmin->modelArr.add(ModelInstance(_model));
		model = AtmoAdmin->modelArr.last;
		model->attribute.entity = this;
		
		AtmoAdmin->physicsArr.add(Physics());
		physics = AtmoAdmin->physicsArr.last;
		physics->attribute.entity = this;
		physics->collider       = _collider;
		physics->position       = _start.position;
		physics->rotation       = _start.rotation;
		physics->scale          = _start.scale;
		physics->mass           = 1.0f;
		physics->staticPosition = true;
		physics->staticRotation = true;
		
		AtmoAdmin->interpTransformArr.add(InterpTransform());
		interp = AtmoAdmin->interpTransformArr.last;
		interp->attribute.entity = this;
		interp->physics  = physics;
		interp->type     = InterpTransformType_Once;
		interp->duration = _duration;
		interp->stages.add(_start);
		interp->stages.add(_end);
		
		id = AtmoAdmin->entities.count;
		AtmoAdmin->entities.add(this);
	}
	
	void ReceiveEvent(Event event)override{
		if(event == Event_ToggleDoor){
			interp->active = !interp->active;
		}
	};
};

#endif //ATMOS_DOORENTITY_H