#pragma once
#ifndef ATTRIBUTE_COLLIDER_H
#define ATTRIBUTE_COLLIDER_H

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


struct Collider : public Attribute {
	ColliderShape shape;
	u32           collLayer;
	mat3          tensor;

	bool noCollide;

	//evenly distributes mass through the respective body
	virtual void RecalculateTensor(f32 mass) {}
};

struct AABBCollider : public Collider {
	vec3 halfDims;

	AABBCollider(Mesh* mesh,    mat3 tensor, u32 collisionLayer = 0, bool nocollide = 0);
	AABBCollider(vec3 halfDims, mat3 tensor, u32 collisionLayer = 0, bool nocollide = 0);
	AABBCollider(Mesh* mesh,    f32 mass,    u32 collisionLayer = 0, bool nocollide = 0);
	AABBCollider(vec3 halfDims, f32 mass,    u32 collisionLayer = 0, bool nocollide = 0);

	void RecalculateTensor(f32 mass) override;
};

struct SphereCollider : public Collider {
	f32 radius;

	SphereCollider(float radius, mat3& tensor, u32 collisionLayer = 0, bool noollide = 0);
	SphereCollider(float radius, f32 mass, mat3& tensor, u32 collisionLayer = 0, bool noollide = 0);

	void RecalculateTensor(f32 mass) override;
};

struct ComplexCollider : public Collider {
	Mesh* mesh;

	ComplexCollider(Mesh* mesh, u32 collisionLayer = 0, bool nocollide = 0);

	//TODO(sushi) implement tensor generation from an arbitrary mesh
	void RecalculateTensor(f32 mass) override;
};

#endif