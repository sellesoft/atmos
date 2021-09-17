#pragma once
#ifndef ATMOS_PHYSICSENTITY_H
#define ATMOS_PHYSICSENTITY_H

#include "Entity.h"

#include "attributes/Physics.h"
#include "attributes/ModelInstance.h"
#include "core/storage.h"

struct PhysicsEntity : 
    public Entity,
    public ModelInstance,
    public Physics {
	//ModelInstance* model;
	//Physics*       physics;
    
	void Init(const char* name, Model* _model, Transform transform, f32 mass, bool static_pos = 0, bool static_rot = 0) {
		Entity::type = (static_pos && static_rot) ? EntityType_StaticMesh : EntityType_NonStaticMesh;
		Entity::name = name;
		Entity::transform = transform;
        
        ModelInstance::ModelInstance((_model) ? _model : Storage::NullModel());

        Physics::collider       = new AABBCollider(_model->mesh, mass);
        Physics::position       = transform.position;
        Physics::rotation       = transform.rotation;
        Physics::scale          = transform.scale;
        Physics::mass           = mass;
        Physics::staticPosition = static_pos;
        Physics::staticRotation = static_rot;
	}
    
	void SendEvent(Event event) override {};
	void ReceiveEvent(Event event) override {};
};

#endif //ATMOS_PHYSICSENTITY_H