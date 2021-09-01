#pragma once
#ifndef SYSTEM_PHYSICS_H
#define SYSTEM_PHYSICS_H



#include "defines.h"

struct PhysicsSystem {
	f32 gravity;
	f32 frictionAir;
	f32 minVelocity;
	f32 maxVelocity;
	f32 minRotVelocity;
	f32 maxRotVelocity; 

	void Init();
	void Update();

};










#endif