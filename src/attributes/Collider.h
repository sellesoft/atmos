#pragma once
#ifndef ATMOS_COLLIDER_H
#define ATMOS_COLLIDER_H

#include "Attribute.h"
#include "math/VectorMatrix.h"

struct Mesh;

enum ColliderShapeBits {
	ColliderShape_NONE,
	ColliderShape_AABB,
	ColliderShape_Sphere,
	ColliderShape_Complex,
	ColliderShape_COUNT,
}; typedef u32 ColliderShape;
global_ const char* ColliderShapeStrings[] = {
	"None", "AABB", "Sphere", "Complex"
};

struct Collider{
	ColliderShape shape;
	vec3 offset;
	mat3 tensor;
	u32  collLayer;
    b32  noCollide;
    
	//evenly distributes mass through the respective body
	virtual void RecalculateTensor(f32 mass){};
};

struct AABBCollider : public Collider{
	vec3 halfDims;
    
	AABBCollider(Mesh* mesh,    f32 mass, vec3 offset = vec3::ZERO, u32 collisionLayer = 0, bool nocollide = 0);
	AABBCollider(vec3 halfDims, f32 mass, vec3 offset = vec3::ZERO, u32 collisionLayer = 0, bool nocollide = 0);
	void RecalculateTensor(f32 mass) override;
};

struct SphereCollider : public Collider{
	f32 radius;
    
	SphereCollider(float radius, f32 mass, vec3 offset = vec3::ZERO, u32 collisionLayer = 0, bool nocollide = 0);
	void RecalculateTensor(f32 mass) override;
};

struct ComplexCollider : public Collider{
	Mesh* mesh;
    
	ComplexCollider(Mesh* mesh, f32 mass, vec3 offset = vec3::ZERO, u32 collisionLayer = 0, bool nocollide = 0);
	//TODO(sushi) implement tensor generation from an arbitrary mesh
	void RecalculateTensor(f32 mass) override {};
};

#endif //ATMOS_COLLIDER_H