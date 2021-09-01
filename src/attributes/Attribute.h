#pragma once
#ifndef ATMOS_ATTRIBUTE_H
#define ATMOS_ATTRIBUTE_H

#include "defines.h"

enum AttributeType_{
	AttributeType_NONE,
	AttributeType_ModelInstance,
	AttributeType_Physics,
	AttributeType_Collider,
	AttributeType_AudioListener,
	AttributeType_AudioSource,
	AttributeType_Light,
	AttributeType_OrbManager,
	AttributeType_Door,
	AttributeType_Player,
	AttributeType_Movement,
	AttributeType_COUNT,
}; typedef u32 AttributeType;
global_ const char* AttributeTypeStrings[] = {
	"None", "ModelInstance", "Physics", "Collider", "AudioListener", "AudioSource", "CameraInstance", "Light", "OrbManager", "Door", "Player", "Movement"
};

struct Entity;
struct Attribute {
	AttributeType type = AttributeType_NONE;
	Entity* entity = 0;
};

#endif //ATMOS_ATTRIBUTE_H