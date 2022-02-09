#pragma once
#ifndef ATMOS_ENTITY_H
#define ATMOS_ENTITY_H

#include "admin.h"
#include "attribute.h"
#include "collider.h"
#include "modelinstance.h"
#include "interptransform.h"
#include "physics.h"
#include "transform.h"
#include "kigu/array.h"
#include "kigu/map.h"
#include "kigu/string.h"
#include "kigu/utils.h"

enum Event_ {
	Event_NONE,
	Event_ModelVisibleToggle,
	Event_PlayerRespawn,
	Event_ToggleTriggerActive,
	Event_ToggleDoor,
	Event_NextLevel,
	Event_COUNT
}; typedef u32 Event;
global_ const char* EventStrings[] = {
	"NONE", "ModelVisibleToggle", "PlayerRespawn", "ToggleTriggerActive", "ToggleDoor", "NextLevel",
};

enum EntityTypeBits {
	EntityType_NONE,
	EntityType_Player,
	EntityType_Physics,
	EntityType_Scenery,
	EntityType_Trigger,
	EntityType_Door,
	EntityType_COUNT
}; typedef u32 EntityType;
global_ const char* EntityTypeStrings[] = {
	"NONE", "Player", "Physics", "Scenery", "Trigger", "Door"
};

struct Entity {
	u32 id;
	EntityType type;
	string name;
	Transform transform;
	
	array<Entity*> connections;
	ModelInstance*    model = 0;
	Physics*        physics = 0;
	InterpTransform* interp = 0;
	
	virtual void SendEvent(Event event){
		for(Entity* e : connections) e->ReceiveEvent(event);
	};
	
	virtual void ReceiveEvent(Event event){
		if(model && event == Event_ModelVisibleToggle) model->visible = !model->visible;
	};
};

//this maybe should be more explicit, we'll see
template<>
struct hash<Entity> {
	inline u32 operator()(Entity& s) {
		return Utils::dataHash32(&s, sizeof(Entity));
	}
};

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

struct PhysicsEntity : public Entity {
	void Init(const char* _name, Transform _transform, Model* _model, Collider _collider, f32 _mass, b32 _static = false){
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

struct PlayerEntity : public Entity {
	f32 standHeight    = 2.0f;
	f32 standEyeLevel  = 1.8f;
	f32 crouchHeight   = 1.2f;
	f32 crouchEyeLevel = 1.1f;
	f32 timeToCrouch   = 0.1f;
	f32 crouchTimer    = 0.0f;
	f32 walkSpeed      = 5.0f;
	f32 runMult        = 2.0f;
	f32 crouchMult     = 0.5f;
	f32 jumpHeight     = 2.0f;
	f32 groundAccel    = 10.0f;
	f32 airAccel       = 10.0f;
	f32 minSpeed       = 0.3f;
	
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
		physics->collider   = AABBCollider(vec3(.5f,standHeight/2.f,.5f), 1.0f);
		physics->position   = _transform.position;
		physics->rotation   = _transform.rotation;
		physics->scale      = _transform.scale;
		physics->mass       = 1.0f;
		physics->elasticity = 0.01f;
		physics->collider.offset = vec3(0,standHeight/2.f,0);
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
		Entity* below = AtmoAdmin->EntityRaycast(physics->position, vec3::DOWN, .05f, 0, true);
		bool inAir = (below == 0);
		
		//apply gravity to velocity
		physics->velocity += vec3(0,-gravity,0) * dt;
		
		if(inAir){
			//cancel horizontal velocity if input is in opposite direction
			if(inputs.angleBetween(physics->velocity.yZero().normalized()) > M_PI*.75f){
				physics->velocity.x = 0;
				physics->velocity.z = 0;
			}
			
			physics->velocity += inputs * airAccel * dt;
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
		f32 crouch_interp = Math::lerp(standHeight, crouchHeight, crouchTimer / timeToCrouch) / 2.f;
		physics->collider.halfDims.y = crouch_interp; 
		physics->collider.offset.y = crouch_interp;
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
			physics->velocity = vec3::ZERO;
			
		}else if(event == Event_NextLevel){
			AtmoAdmin->loadNextLevel = true;
		}
	};
};


//mesh that the player should never interact with but can see
struct SceneryEntity : public Entity {
	void Init(const char* _name, Transform _transform, Model* _model) {
		type = EntityType_Scenery;
		name = _name;
		transform = _transform;
		
		Assert(_model);
		AtmoAdmin->modelArr.add(ModelInstance(_model));
		model = AtmoAdmin->modelArr.last;
		model->attribute.entity = this;
		
		id = AtmoAdmin->entities.count;
		AtmoAdmin->entities.add(this);
	}
};

struct TriggerEntity : public Entity {
	array<Event> events;
	
	void Init(const char* _name, Transform _transform, Collider _collider, Model* _model = 0){
		type = EntityType_Trigger;
		name = _name;
		transform = _transform;
		
		if(_model){
			AtmoAdmin->modelArr.add(ModelInstance(_model));
			model = AtmoAdmin->modelArr.last;
			model->attribute.entity = this;
		}
		
		AtmoAdmin->physicsArr.add(Physics());
		physics = AtmoAdmin->physicsArr.last;
		physics->attribute.entity = this;
		physics->collider       = _collider;
		physics->position       = _transform.position;
		physics->rotation       = _transform.rotation;
		physics->scale          = _transform.scale;
		physics->mass           = 1.0f;
		physics->staticPosition = true;
		physics->staticRotation = true;
		physics->collider.isTrigger = true;
		physics->collider.noCollide = true;
		
		id = AtmoAdmin->entities.count;
		AtmoAdmin->entities.add(this);
		AtmoAdmin->triggers.add(this);
	}
	
	void Update(){
		if(physics->collider.triggerActive){ forI(events.count){ SendEvent(events[i]); } }
		physics->collider.triggerActive = false;
	}
	
	void ReceiveEvent(Event event)override{
		if      (event == Event_ToggleTriggerActive){
			physics->collider.isTrigger = !physics->collider.isTrigger;
		}else if(model && event == Event_ModelVisibleToggle){
			model->visible = !model->visible;
		}
	};
};

#endif //ATMOS_ENTITY_H