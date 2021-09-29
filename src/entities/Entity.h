#pragma once
#ifndef ATMOS_ENTITY_H
#define ATMOS_ENTITY_H

#include "../admin.h"
#include "../transform.h"
#include "../attributes/Attribute.h"
#include "utils/string.h"
#include "utils/array.h"
#include "utils/map.h"
#include "utils/utils.h"

struct Player;
struct Movement;
struct ModelInstance;
struct Physics;
struct Collider;
struct InterpTransform;

enum Event_ {
	Event_NONE,
	Event_ModelVisibleToggle,
	Event_PlayerRespawn,
	Event_ToggleTriggerActive,
	Event_ToggleDoor,
	Event_COUNT
}; typedef u32 Event;
global_ const char* EventStrings[] = {
	"NONE", "ModelVisibleToggle", "PlayerRespawn", "ToggleTriggerActive", "ToggleDoor",
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

#endif //ATMOS_ENTITY_H