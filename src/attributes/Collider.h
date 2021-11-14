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

namespace InertiaTensors {
	static mat3 SolidSphere(float radius, float mass) {
		float value = .4f * mass * radius * radius;
		return mat3(value, 0, 0,
					0, value, 0,
					0, 0, value);
	}
	
	static mat3 HollowSphere(float radius, float mass) {
		float value = (2.f/3.f) * mass * radius * radius;
		return mat3(value, 0, 0,
					0, value, 0,
					0, 0, value);
	}
	
	static mat3 SolidEllipsoid(vec3 halfDimensions, float mass) {
		float oneFifthMass = .2f * mass;
		float aSqrd = halfDimensions.x * halfDimensions.x;
		float bSqrd = halfDimensions.y * halfDimensions.y;
		float cSqrd = halfDimensions.z * halfDimensions.z;
		return mat3(oneFifthMass* (bSqrd + cSqrd), 0, 0,
					0, oneFifthMass* (bSqrd + cSqrd), 0,
					0, 0, oneFifthMass* (bSqrd + cSqrd));
	}
	
	static mat3 SolidCuboid(float width, float height, float depth, float mass) {
		float oneTwelfthMass = (1.f/12.f) * mass;
		float wSqrd = width * width;
		float hSqrd = height * height;
		float dSqrd = depth * depth;
		return mat3(oneTwelfthMass* (hSqrd + dSqrd), 0, 0,
					0, oneTwelfthMass* (wSqrd + dSqrd), 0,
					0, 0, oneTwelfthMass* (wSqrd + hSqrd));
	}
	
	static mat3 SolidCylinder(float radius, float height, float mass) {
		float rSqrd = radius * radius;
		float value = (1.f/12.f) * mass * (3 * rSqrd + height * height);
		return mat3(value, 0, 0,
					0, value, 0,
					0, 0, mass*rSqrd/2.f);
	}
};

#endif //ATMOS_COLLIDER_H