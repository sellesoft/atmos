#pragma once
#ifndef ATMOS_PLAYERENTITY_H
#define ATMOS_PLAYERENTITY_H

#include "Entity.h"
#include "attributes/Physics.h"
#include "attributes/Collider.h"
#include "attributes/ModelInstance.h"
#include "math/math.h"
#include "core/logging.h"

struct PlayerEntity : public Entity {
	f32 standHeight    = 2.0;
	f32 standEyeLevel  = 1.8;
	f32 crouchHeight   = 1.2;
	f32 crouchEyeLevel = 1.1;
	f32 timeToCrouch   = 0.1;
	f32 crouchTimer    = 0.0;
	f32 walkSpeed      = 5.0;
	f32 runMult        = 2.0;
	f32 crouchMult     = 0.5;
	f32 jumpHeight     = 2.0;
	f32 groundAccel    = 10.0;
	f32 airAccel       = 10.0;
	f32 minSpeed       = 0.3;
	
	b32 isJumping   = false;
	b32 isCrouching = false;
	b32 isRunning   = false;
	
	Transform spawnpoint;
	vec3 inputs = vec3::ZERO;
	
	void Init(Transform _transform = Transform()) {
		type = EntityType_Player;
		name = "player";
		transform = _transform;
		
		AtmoAdmin->modelArr.add(ModelInstance());
		model = AtmoAdmin->modelArr.last;
		model->attribute.entity = this;
		
		AtmoAdmin->physicsArr.add(Physics());
		physics = AtmoAdmin->physicsArr.last;
		physics->attribute.entity = this;
		physics->collider   = new AABBCollider(vec3(.5f,standHeight/2.f,.5f), 1.0);
		physics->position   = _transform.position;
		physics->rotation   = _transform.rotation;
		physics->scale      = _transform.scale;
		physics->mass       = 1.0;
		physics->elasticity = 0.01;
		physics->collider->offset = vec3(0,standHeight/2.f,0);
		spawnpoint = _transform;
		
		id = AtmoAdmin->entities.count;
		AtmoAdmin->entities.add(this);
	}
	
	void Update(){
		f32 gravity = AtmoAdmin->physics.gravity;
		f32 dt = AtmoAdmin->physics.fixedDeltaTime;
		f32 max_speed = walkSpeed;
		
		//TODO limit crouch if stuck under something
		if(isCrouching){
			if(crouchTimer < timeToCrouch) crouchTimer += dt;
			max_speed *= crouchMult;
		}else{
			if(crouchTimer > 0) crouchTimer -= dt;
			if(isRunning){
				max_speed *= runMult;
			}
		}
		
		//check if in air or on ground
		Entity* below = AtmoAdmin->EntityRaycast(physics->position, vec3::DOWN, .05f);
		bool inAir = (below == 0);
		
		//apply gravity to velocity
		physics->velocity += vec3(0,-gravity,0) * dt;
		
		if(inAir){
			physics->velocity += inputs * airAccel * dt;
			vec3 horiz_clamp = physics->velocity; horiz_clamp.y = 0; horiz_clamp.clampMag(0, max_speed);
			physics->velocity = vec3(horiz_clamp.x, physics->velocity.y, horiz_clamp.z);
		}else{
			physics->velocity += inputs * groundAccel * dt;
			physics->velocity.y = 0; 
			f32 vel_mag  = physics->velocity.mag();
			vec3 vel_dir = physics->velocity.normalized();
			if(vel_mag > max_speed){
				physics->velocity = vel_dir * max_speed;
				vel_mag = max_speed;
			}
			
			//ground friction
			if(inputs == vec3::ZERO){
				if(vel_mag > minSpeed){
					//TODO get normal from collision
					//vec3 vPerpNorm = physics->velocity - (vec3::UP * physics->velocity.dot(vec3::UP));
					physics->acceleration += vel_dir * (physics->kineticFricCoef * -gravity) * vel_mag * 2;
					physics->velocity += physics->acceleration * dt;
				}else{
					physics->velocity = vec3::ZERO;
				}
			}
			
			if(isJumping){
				physics->velocity.y = sqrt(2.f*gravity*jumpHeight);
				inAir = true;
			}else{
				physics->velocity.y = 0;
			}
		}
		
		physics->position += physics->velocity * dt;
		physics->acceleration = vec3::ZERO;
		
		AABBCollider* collider = (AABBCollider*)physics->collider;
		f32 crouch_interp = Math::lerp(standHeight, crouchHeight, crouchTimer / timeToCrouch) / 2.f;
		collider->halfDims.y = crouch_interp; collider->offset.y = crouch_interp;
	}
	
	void PostCollisionUpdate(){
		AtmoAdmin->camera.position = vec3(physics->position.x, 
										  physics->position.y+Math::lerp(standEyeLevel, crouchEyeLevel, crouchTimer / timeToCrouch),
										  physics->position.z);
	}
	
	void ReceiveEvent(Event event)override{
		if      (event == Event_ModelVisibleToggle){
			model->visible = !model->visible;
		}else if(event == Event_PlayerRespawn){
			transform         = spawnpoint;
			physics->position = spawnpoint.position;
			physics->rotation = spawnpoint.rotation;
			physics->scale    = spawnpoint.scale;
		}
	};
};

#endif //ATMOS_PLAYERENTITY_H