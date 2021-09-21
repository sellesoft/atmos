#pragma once
#ifndef ATMOS_TRIGGERENTITY_H
#define ATMOS_TRIGGERENTITY_H

#include "Entity.h"
#include "attributes/Physics.h"
#include "attributes/ModelInstance.h"
#include "core/storage.h"

struct TriggerEntity : public Entity {
	ModelInstance* model;
	Physics*       physics;
	array<Event>   events;
    
	void Init(const char* _name, Transform _transform, Collider* _collider, Model* _model = 0){
		type = EntityType_Trigger;
		name = _name;
		transform = _transform;
        
		if(_model){
			AtmoAdmin->modelArr.add(ModelInstance(_model));
			model = AtmoAdmin->modelArr.last; modelPtr = model;
			model->attribute.entity = this;
		}
        
		Assert(_collider);
        AtmoAdmin->physicsArr.add(Physics());
        physics = AtmoAdmin->physicsArr.last; physicsPtr = physics;
		physics->attribute.entity = this;
        physics->collider       = _collider;
        physics->position       = _transform.position;
        physics->rotation       = _transform.rotation;
        physics->scale          = _transform.scale;
		physics->mass           = 1.0f;
		physics->staticPosition = true;
		physics->staticRotation = true;
		physics->collider->isTrigger = true;
		physics->collider->noCollide = true;
		
		AtmoAdmin->entities.add(this);
		AtmoAdmin->triggers.add(this);
	}
    
	void Update(){
		if(physics->collider->triggerActive) forI(events.count) SendEvent(events[i]);
		physics->collider->triggerActive = false;
	}
	
	void ReceiveEvent(Event event)override{
		if      (event == Event_ToggleTriggerActive){
			physics->collider->isTrigger = !physics->collider->isTrigger;
		}else if(event == Event_ModelVisibleToggle){
			modelPtr->visible = !modelPtr->visible;
		}
	};
};

#endif //ATMOS_TRIGGERENTITY_H