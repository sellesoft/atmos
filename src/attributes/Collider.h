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
	vec3 point0; //local to p0
	vec3 point1; //local to p1
	vec3 normal; //0 to 1
	f32  penetration;
};

struct Manifold{
	Entity   *e0, *e1;
	Physics  *p0, *p1;
	Collider *c0, *c1;
	Contact contacts[4];
	u32 contact_count;
	Type state;
};

struct Collider{
	Type type = ColliderType_NONE;
	mat3 tensor{};
	vec3 offset{};
	u32  layer = 0;
	b32  noCollide  = false;
	b32  isTrigger  = false;
	b32  playerOnly = false;
	
	b32 triggerActive = false; //TODO replace this with manifold stuffs
	
	//evenly distributes mass through the respective body
	virtual void RecalculateTensor(f32 mass) = 0;
};

struct AABBCollider : public Collider{
	vec3 halfDims;
	
	AABBCollider(Mesh* mesh,    f32 mass);
	AABBCollider(vec3 halfDims, f32 mass);
	void RecalculateTensor(f32 mass) override;
};

struct SphereCollider : public Collider{
	f32 radius;
	
	SphereCollider(f32 radius, f32 mass);
	void RecalculateTensor(f32 mass) override;
};

#endif //ATMOS_COLLIDER_H