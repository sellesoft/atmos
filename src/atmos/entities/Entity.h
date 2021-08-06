#pragma once

#ifndef ATMOS_ENTITY_H
#define ATMOS_ENTITY_H

#include "utils/string.h"
#include "utils/array.h"
#include "utils/map.h"
#include "utils/utils.h"
#include "../Transform.h"
#include "../attributes/Attribute.h"

enum Event_ {
	Event_NONE = 0,
	Event_DoorToggle,
	Event_LightToggle,
	Event_ModelVisibleToggle,
}; typedef u32 Event;

global_ const char* EventStrings[] = {
	"NONE", "DoorToggle", "LightToggle", "ModelVisibleToggle",
};

enum EntityType_ {
	EntityType_Anonymous,
	EntityType_Player,
	EntityType_StaticMesh,
	EntityType_Trigger,
	EntityType_COUNT,
}; typedef u32 EntityType;

global_ const char* EntityTypeStrings[] = {
	"Anonymous", "Player", "StaticMesh", "Trigger"
};

struct Entity {
	string name;
	u32 id; //do ents need ids anymore?

	EntityType type;

	Transform transform;

	set<Entity*> connections;

	array<Attribute*> attributes;
	
	virtual void SendEvent(Event event) {};
	virtual void ReceiveEvent(Event event) {};
};

//this maybe should be more explicit, we'll see
template<>
struct hash<Entity> {
	inline u32 operator()(Entity s) {
		return Utils::dataHash32(&s, sizeof(Entity));
	}
};

#endif