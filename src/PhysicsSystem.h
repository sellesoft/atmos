#pragma once
#ifndef ATMOS_PHYSICSSYSTEM_H
#define ATMOS_PHYSICSSYSTEM_H

#include "defines.h"

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
	
	void Init(f32 fixedUpdatesPerSecond);
	void Update();
};

#endif //ATMOS_PHYSICSSYSTEM