#include "Movement.h"
#include "Physics.h"
#include "ModelInstance.h"
#include "CameraInstance.h"
#include "admin.h"
#include "core/window.h"
#include "core/time.h"

Movement::Movement() {
	type = AttributeType_Movement;
}

Movement::Movement(Physics* phys) {
	type = AttributeType_Movement;
	this->phys = phys;
	phys->kineticFricCoef = 4;
}

//for loading
Movement::Movement(Physics* phys, float gndAccel, float airAccel, float maxWalkingSpeed, float maxRunningSpeed, float maxCrouchingSpeed, bool jump, float jumpImpulse) {
	type = AttributeType_Movement;
	this->phys = phys;
	//phys->kineticFricCoef = 1;
	//phys->physOverride = true;
	this->gndAccel = gndAccel;
	this->airAccel = airAccel;
	this->maxWalkingSpeed = maxWalkingSpeed;
	this->maxRunningSpeed = maxRunningSpeed;
	this->maxCrouchingSpeed = maxCrouchingSpeed;
	this->jump = jump;
	this->jumpImpulse = jumpImpulse;
}

void Movement::DecideMovementState() {

}

void Movement::GrabObject() {

}


void Movement::Update() {

}
