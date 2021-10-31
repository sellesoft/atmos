#pragma once
#ifndef ATMOS_COLLIDER_H
#define ATMOS_COLLIDER_H

#include "Attribute.h"
#include "math/math.h"

struct Physics;
struct Collider;
struct Mesh;

enum ColliderType{
	ColliderType_NONE,
	ColliderType_AABB,
	ColliderType_Sphere,
	ColliderType_Hull,
	ColliderType_COUNT,
};
global_ const char* ColliderTypeStrings[] = {
	"None", "AABB", "Sphere", "Hull"
};

struct Collider{
	Type type = ColliderType_NONE;
	mat3 tensor;
	vec3 offset{};
	u32  layer = 0;
	b32  noCollide = false;
	b32  isTrigger = false;
	b32  playerOnly = false;
	b32  triggerActive = false; //TODO replace this with manifold stuffs
	union{
		vec3  halfDims;
		f32   radius;
		Mesh* mesh;
	};
	
	Collider(){}
	Collider(const Collider& rhs);
	void operator= (const Collider& rhs);
	//evenly distributes mass through the respective body
	void RecalculateTensor(f32 mass);
};

Collider AABBCollider(Mesh* mesh, f32 mass);
Collider AABBCollider(vec3 halfDimensions, f32 mass);
Collider SphereCollider(f32 radius, f32 mass);
Collider HullCollider(Mesh* mesh, f32 mass);

#endif //ATMOS_COLLIDER_H