#pragma once
#ifndef ATMOS_PLAYERENTITY_H
#define ATMOS_PLAYERENTITY_H

#include "Entity.h"
#include "attributes/Player.h"
#include "attributes/Physics.h"
#include "attributes/Movement.h"
#include "attributes/ModelInstance.h"
#include "core/storage.h"

struct PlayerEntity : 
	public Entity, 
	public Player, 
	public Movement,
	public ModelInstance, 
	public Physics {
	//ModelInstance* model;
	//Physics*       physics;
	
	f32 walkSpeed   = 5.f;
	f32 sprintMult  = 2.f;
	f32 crouchMult  = .5f;
	f32 jumpImpulse = 10.f;
	
	b32 isJumping   = false;
	b32 isCrouching = false;
	b32 isRunning   = false;
	
	vec3 inputs;
    
	void Init(const char* name, Mesh* mesh, Transform transform, f32 mass) {
		Entity::type = EntityType_Player;
		Entity::name = name;
		Entity::transform = transform;
        
		ModelInstance::mesh = (mesh) ? mesh : Storage::NullMesh();
        
		Physics::collider   = new AABBCollider(ModelInstance::mesh, mass);
        Physics::position   = transform.position;
        Physics::rotation   = transform.rotation;
        Physics::scale      = transform.scale;
        Physics::mass       = mass;
        Physics::elasticity = 0;
	}
	
	void Update(){
		Player::       Update();
		Movement::     Update();
		ModelInstance::Update();
	}
	
	void SendEvent(Event event) override {};
	void ReceiveEvent(Event event) override {};
};


#endif //ATMOS_PLAYERENTITY_H