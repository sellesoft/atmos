#pragma once
#ifndef ATMOS_PHYSICSSYSTEM_H
#define ATMOS_PHYSICSSYSTEM_H

#include "defines.h"
#include "math/math.h"
#include "utils/array.h"

struct Physics;
struct Collider;

struct Contact{
	//computed during detection
	vec3 world; //incident point on face vs other, midpoint on curve vs curve
	vec3 local0;
	vec3 local1;
	f32  penetration; //always negative
	
	//computed during resolution
	vec3 tangent0, tangent1;
	vec3 normalCrossLocal0, normalCrossLocal1;
	vec3 tangent0CrossLocal0, tangent0CrossLocal1;
	vec3 tangent1CrossLocal0, tangent1CrossLocal1;
	f32  normalMass; //effective mass along normal
	f32  tangentMass0, tangentMass1; //effective mass along tangents
	f32  velocityBias; //restitution based bias
	f32  normalImpulse; //non-penetration impulse
	f32  tangentImpulse0, tangentImpulse1; //friction impulses
};

struct Manifold{
	//computed during detection
	Physics  *p0, *p1;
	Collider *c0, *c1;
	vec3 normal; //from p0 to p1
	u32 contactCount; //collision exists if -1, but dont resolve
	Contact contacts[4]; //four contact points should be stable enough
	
	//computed during resolution
	f32  friction; //mixed friction between both bodies
	f32  invMass0, invMass1;
	mat3 invInertia0, invInertia1; //in world space
};

enum ConstraintType{
	ConstraintType_NONE,
	ConstraintType_Friction,
	ConstraintType_Distance,
	ConstraintType_Revolute,  //rotation around a single point
	ConstraintType_Prismatic, //sliding along a single axis
	ConstraintType_Pulley,    //idealized pulley
	ConstraintType_COUNT
};

struct Constraint{
	Type type;
};

struct PhysicsSystem {
	//world properties
	f32 gravity;
	
	//time step
	f32 fixedTimeStep;
	f32 fixedDeltaTime;
	f64 fixedTotalTime;
	u64 fixedUpdateCount;
	f32 fixedAccumulator;
	f32 fixedAlpha;
	
	//subsystem toggles
	b32 paused;
	b32 integrating;
	b32 solving;
	b32 step;
	
	//integrating tweaks
	f32 minVelocity;
	f32 maxVelocity;
	f32 minRotVelocity;
	f32 maxRotVelocity;
	
	//solving tweaks
	u32 velocityIterations;
	u32 positionIterations;
	f32 baumgarte;
	f32 linearSlop; //linear collision and constraint tolerance
	f32 angularSlop; //in radians
	f32 restitutionSlop; //max allowed restitution velocity
	f32 maxLinearCorrection;
	
	void Init(f32 fixedUpdatesPerSecond);
	void Update();
};

#endif //ATMOS_PHYSICSSYSTEM