#pragma once
#ifndef ATTRIBUTE_MOVEMENT_H
#define ATTRIBUTE_MOVEMENT_H

#include "Attribute.h"
#include "math/Vector.h"

enum MoveState : u32 {
	InAirNoInput, // this isn't necessary i dont think
	InAirCrouching,
	OnGroundNoInput,
	OnGroundWalking,
	OnGroundRunning,
	OnGroundCrouching,
	OnGroundCrouched
};

struct Physics;
struct Camera;

struct Movement {
	Attribute attribute{ AttributeType_Movement };
	
	vec3 inputs;
	Physics* phys;
	
	Camera* camera;
	
	bool inAir;
	MoveState moveState;
	
	float gndAccel = 100;
	float airAccel = 1000;
	
	float jumpImpulse = 10;
	
	float maxWalkingSpeed = 5;
	float maxRunningSpeed = 12;
	float maxCrouchingSpeed = 2.5;
	
	bool jump = false;
	
	float maxGrabbingDistance = 5;
	
	Movement();
	Movement(Physics* phys);
	Movement(Physics* phys, float gndAccel, float airAccel, float maxWalkingSpeed, float maxRunningSpeed, float maxCrouchingSpeed, bool jump, float jumpImpulse);
	
	void Update();
	void DecideMovementState();
	void GrabObject();
};

#endif //ATTRIBUTE_MOVEMENT_H