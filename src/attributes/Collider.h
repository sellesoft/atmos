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

enum ContactState_{
	ContactState_NONE,
	ContactState_Stationary,
	ContactState_Moving,
}; typedef u32 ContactState;

enum ColliderShapeBits {
	ColliderShape_NONE,
	ColliderShape_AABB,
	ColliderShape_Sphere,
	ColliderShape_COUNT,
}; typedef u32 ColliderShape;
global_ const char* ColliderShapeStrings[] = {
	"None", "AABB", "Sphere"
};

struct Contact{
	vec3 point;
	f32  penetration;
};

struct Manifold{
	Entity*   e1 = 0;
	Entity*   e2 = 0;
	Physics*  p1 = 0;
	Physics*  p2 = 0;
	Collider* c1 = 0;
	Collider* c2 = 0;
	vec3 normal{};
	ContactState state = ContactState_NONE;
	array<Contact> contacts;
};

struct Collider{
	ColliderShape shape = ColliderShape_NONE;
	mat3 tensor{};
	vec3 offset{};
	u32  layer = 0;
	b32  noCollide = false;
	b32  isTrigger = false;
	
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