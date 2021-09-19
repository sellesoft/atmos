#pragma once
#ifndef ATMOS_ENTITY_H
#define ATMOS_ENTITY_H

#include "../admin.h"
#include "../attributes/Attribute.h"
#include "utils/string.h"
#include "utils/array.h"
#include "utils/map.h"
#include "utils/utils.h"
#include "math/VectorMatrix.h"

enum Event_ {
	Event_NONE,
	Event_ModelVisibleToggle,
	Event_PlayerRespawn,
	Event_ToggleTriggerActive,
	Event_COUNT
}; typedef u32 Event;
global_ const char* EventStrings[] = {
	"NONE", "ModelVisibleToggle", "PlayerRespawn", "ToggleTriggerActive",
};

enum EntityTypeBits {
	EntityType_Anonymous,
	EntityType_Player,
	EntityType_Physics,
	EntityType_Scenery,
	EntityType_Trigger,
	EntityType_COUNT
}; typedef u32 EntityType;
global_ const char* EntityTypeStrings[] = {
	"Anonymous", "Player", "Physics", "Scenery", "Trigger"
};

struct Transform{
	vec3 position = vec3::ZERO;
	vec3 rotation = vec3::ZERO;
	vec3 scale    = vec3::ONE;
	vec3 prevPosition = vec3::ZERO;
	vec3 prevRotation = vec3::ZERO;
	vec3 prevScale    = vec3::ONE;
    
	Transform(vec3 pos=vec3::ZERO, vec3 rot=vec3::ZERO, vec3 _scale=vec3::ONE,
			  vec3 prevPos=vec3::ZERO, vec3 prevRot=vec3::ZERO, vec3 _prevScale=vec3::ZERO){
		position = pos; rotation = rot; scale = _scale; prevPosition = prevPos; prevRotation = prevRot; prevScale = _prevScale;
	}
	inline vec3 Up()     { return vec3::UP      * mat4::RotationMatrix(rotation); }
	inline vec3 Right()  { return vec3::RIGHT   * mat4::RotationMatrix(rotation); }
	inline vec3 Forward(){ return vec3::FORWARD * mat4::RotationMatrix(rotation); }
	inline mat4 Matrix() { return mat4::TransformationMatrix(position, rotation, scale); }
};

struct Player;
struct Movement;
struct ModelInstance;
struct Physics;
struct Collider;

struct Entity {
	string name;
	EntityType type;
	Transform transform;
    
	Player*       playerPtr = nullptr;
	Physics*     physicsPtr = nullptr;
	Movement*   movementPtr = nullptr;
	ModelInstance* modelPtr = nullptr;
    
	array<Entity*> connections;
	
	virtual void SendEvent(Event event){
		for(Entity* e : connections) e->ReceiveEvent(event);
	};
	
	virtual void ReceiveEvent(Event event){
		if(modelPtr && event == Event_ModelVisibleToggle) modelPtr->visible = !modelPtr->visible;
	};
};

//this maybe should be more explicit, we'll see
template<>
struct hash<Entity> {
	inline u32 operator()(Entity& s) {
		return Utils::dataHash32(&s, sizeof(Entity));
	}
};

#endif //ATMOS_ENTITY_H