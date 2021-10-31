#pragma once
#ifndef ATMOS_PHYSICSSYSTEM_H
#define ATMOS_PHYSICSSYSTEM_H

#include "defines.h"
#include "math/math.h"

struct Physics;
struct Collider;

struct Contact{
	vec3 world; //incident point on face vs other, midpoint on curve vs curve
	vec3 local0;
	vec3 local1;
	f32  penetration; //always negative
};

struct Manifold{
	Physics  *p0, *p1;
	Collider *c0, *c1;
	vec3 normal; //from p0 to p1
	vec3 tangent0, tangent1;
	f32 normalImpulseSum; //non-penetration impulse
	f32 tangentImpulseSum0, tangentImpulseSum1; //friction impulse
	u32 contactCount; //collision exists if -1, but dont resolve
	Contact contacts[4]; //four contact points should be stable enough
};

enum ConstraintType{ //DOF = degrees of freedom
	ConstraintType_NONE,
	ConstraintType_Friction,
	ConstraintType_Distance,
	ConstraintType_Revolute,  //rotation around a single point, 1 DOF
	ConstraintType_Prismatic, //sliding along a single axis, 1 DOF
	ConstraintType_Pulley,    //idealized pulley
	ConstraintType_COUNT
};

struct Constraint{
	Type type;
};

struct PhysicsSystem {
	f32 gravity;
	f32 minVelocity;
	f32 maxVelocity;
	f32 minRotVelocity;
	f32 maxRotVelocity; 
	
	f32 fixedTimeStep;
	f32 fixedDeltaTime;
	f64 fixedTotalTime;
	u64 fixedUpdateCount;
	f32 fixedAccumulator;
	f32 fixedAlpha;
	
	b32 paused;
	b32 integrating;
	b32 solving;
	b32 step;
	
	void Init(f32 fixedUpdatesPerSecond);
	void Update();
};

#endif //ATMOS_PHYSICSSYSTEM