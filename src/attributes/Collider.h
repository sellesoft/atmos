#pragma once
#ifndef ATMOS_COLLIDER_H
#define ATMOS_COLLIDER_H

#include "Attribute.h"
#include "math/VectorMatrix.h"
#include "utils/array.h"

struct Entity;
struct Physics;
struct Collider;
struct Mesh;

enum ContactState{
	ContactState_NONE,
	ContactState_Stationary,
	ContactState_Moving,
};

enum ColliderType{
	ColliderType_NONE,
	ColliderType_AABB,
	ColliderType_Sphere,
	ColliderType_COUNT,
};
global_ const char* ColliderTypeStrings[] = {
	"None", "AABB", "Sphere"
};

struct Contact{
	vec3 local0; //local to p0
	vec3 local1; //local to p1
	vec3 normal; //1 to 0
	f32  penetration;
};

struct Manifold{
	Entity   *e0, *e1;
	Physics  *p0, *p1;
	Collider *c0, *c1;
	Contact contacts[4];
	u32 contactCount;
	Type state;
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
	struct{
		union{
			vec3 halfDims;
			f32  radius;
		};
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

#endif //ATMOS_COLLIDER_H