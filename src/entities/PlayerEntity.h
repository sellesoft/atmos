#pragma once
#ifndef ATMOS_PLAYERENTITY_H
#define ATMOS_PLAYERENTITY_H

#include "Entity.h"
#include "attributes/Physics.h"
#include "attributes/ModelInstance.h"
#include "math/math.h"
#include "core/logging.h"

struct PlayerEntity : public Entity {
	ModelInstance* model;
	Physics*       physics;
	
	f32 standHeight    = 2.0;
	f32 standEyeLevel  = 1.8;
	f32 crouchHeight   = 1.2;
	f32 crouchEyeLevel = 1.1;
	f32 timeToCrouch   = 0.2;
	f32 crouchTimer    = 0.0;
	f32 walkSpeed      = 5.0;
	f32 runMult        = 2.0;
	f32 crouchMult     = 0.5;
	f32 jumpImpulse    = 2.0;
	f32 groundAccel    = 10.0;
	f32 airAccel       = 100.0;
	f32 minSpeed       = 0.12;
	
	b32 isJumping   = false;
	b32 isCrouching = false;
	b32 isRunning   = false;
	
	vec3 inputs;
    
	void Init(const char* name, Transform transform, f32 mass) {
		type = EntityType_Player;
		this->name = name;
		this->transform = transform;
        
        AtmoAdmin->modelArr.add(ModelInstance());
		model = AtmoAdmin->modelArr.last; modelPtr = model;
		model->attribute.entity = this;
        
        AtmoAdmin->physicsArr.add(Physics());
        physics = AtmoAdmin->physicsArr.last; physicsPtr = physics;
		physics->attribute.entity = this;
        physics->collider   = new AABBCollider(vec3(.5f,standHeight/2.f,.5f), mass, vec3(0,standHeight/2.f,0));
        physics->position   = transform.position;
        physics->rotation   = transform.rotation;
        physics->scale      = transform.scale;
        physics->mass       = mass;
        physics->elasticity = 0;
		
		AtmoAdmin->entities.add(this);
	}
	
	void Update(){
		f32 gravity = AtmoAdmin->physics.gravity;
		f32 dt = AtmoAdmin->physics.fixedDeltaTime;
		AABBCollider* collider = (AABBCollider*)physics->collider;
		f32 max_speed = walkSpeed;
		
		if(isCrouching){
			if(crouchTimer < timeToCrouch) crouchTimer += dt;
			max_speed *= crouchMult;
		}else{
			if(crouchTimer > 0) crouchTimer -= dt;
			if(isRunning){
				max_speed *= runMult;
			}
		}
		
		//apply gravity to velocity
		physics->velocity += vec3(0,-gravity,0) * dt;
		
		//check if in air or on ground
		Entity* below = AtmoAdmin->EntityRaycast(physics->position, vec3::DOWN, .25f);
		bool inAir = (below == 0);
		if(below) Logf("","%s",below->name);
		
		if(inAir){
			physics->velocity += inputs * airAccel * dt;
		}else{
			physics->velocity += inputs * groundAccel * dt;
			physics->velocity.clampMag(0, max_speed);
			
			//ground friction
			if(inputs != vec3::ZERO){
				if(physics->velocity.mag() > minSpeed){
					//TODO get normal from collision
					vec3 vPerpNorm = physics->velocity - (vec3::UP * physics->velocity.dot(vec3::UP));
					physics->acceleration += vPerpNorm.normalized() * (physics->kineticFricCoef * -gravity);
					physics->velocity += physics->acceleration * dt;
				}else{
					physics->velocity = vec3::ZERO;
				}
			}
			
			if(isJumping){
				physics->AddImpulse(vec3(0,jumpImpulse,0));
				inAir = true;
			}
		}
		
		physics->position += physics->velocity * dt;
		physics->acceleration = vec3::ZERO;
		
		f32 crouch_interp = Math::lerpf(standHeight, crouchHeight, crouchTimer / timeToCrouch) / 2.f;
		collider->halfDims.y = crouch_interp; collider->offset.y = crouch_interp;
		AtmoAdmin->camera.position = vec3(physics->position.x, 
										  physics->position.y+Math::lerpf(standEyeLevel, crouchEyeLevel, crouchTimer / timeToCrouch),
										  physics->position.z);
	}
	
	void SendEvent(Event event) override {};
	void ReceiveEvent(Event event) override {};
};


#endif //ATMOS_PLAYERENTITY_H