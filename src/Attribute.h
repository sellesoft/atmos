#pragma once
#ifndef ATMOS_ATTRIBUTE_H
#define ATMOS_ATTRIBUTE_H

#include "kigu/common.h"
#include "kigu/string.h"

enum AttributeType_{
	AttributeType_NONE,
	AttributeType_ModelInstance,
	AttributeType_Physics,
	AttributeType_Player,
	AttributeType_Movement,
	AttributeType_InterpTransform,
	AttributeType_COUNT,
}; typedef u32 AttributeType;
global_ const char* AttributeTypeStrings[] = {
	"None", "ModelInstance", "Physics", "Player", "Movement", "InterpTransform",
};

struct Entity;
struct Attribute {
	AttributeType type = AttributeType_NONE;
	Entity* entity = 0;
};

#endif //ATMOS_ATTRIBUTE_H