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
	Event_NONE = 0,
	Event_DoorToggle,
	Event_LightToggle,
	Event_ModelVisibleToggle,
}; typedef u32 Event;
global_ const char* EventStrings[] = {
	"NONE", "DoorToggle", "LightToggle", "ModelVisibleToggle",
};

enum EntityTypeBits {
	EntityType_Anonymous     = 1 << 0,
	EntityType_Player        = 1 << 1,
	EntityType_NonStaticMesh = 1 << 2,
	EntityType_StaticMesh    = 1 << 3,
	EntityType_SceneryMesh   = 1 << 4,
	EntityType_Trigger       = 1 << 5,
}; typedef u32 EntityType;
global_ const char* EntityTypeStrings[] = {
	"Anonymous", "Player", "StaticMesh", "Trigger"
};

struct Transform{
	vec3 position = vec3::ZERO;
	vec3 rotation = vec3::ZERO;
	vec3 scale    = vec3::ONE;
	vec3 prevPosition = vec3::ZERO;
	vec3 prevRotation = vec3::ZERO;
	vec3 prevScale    = vec3::ONE;
	
	//Transform(vec3 position = vec3::ZERO, vec3 rotation = vec3::ZERO, vec3 scale = vec3::ONE);
    
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
    
	set<Entity*> connections;
	
	virtual void SendEvent(Event event) {};
	virtual void ReceiveEvent(Event event) {};
};

//this maybe should be more explicit, we'll see
template<>
struct hash<Entity> {
	inline u32 operator()(Entity& s) {
		return Utils::dataHash32(&s, sizeof(Entity));
	}
};

#endif //ATMOS_ENTITY_H