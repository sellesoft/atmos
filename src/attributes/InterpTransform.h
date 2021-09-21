#pragma once
#ifndef ATTRIBUTE_INTERPTRANSFORM_H
#define ATTRIBUTE_INTERPTRANSFORM_H

#include "Attribute.h"
#include "Physics.h"
#include "../transform.h"
#include "math/math.h"
#include "utils/array.h"

enum InterpTransformType{
	InterpTransformType_Once,
	InterpTransformType_Bounce,
};

struct InterpTransform{
	Attribute attribute{ AttributeType_InterpTransform };
	Physics* physics = 0;
	Type type = InterpTransformType_Once;
	f32 duration = 0;
	f32 current = 0;
	b32 active = false;
	array<Transform> stages;
	
	void Update(){
		Assert(physics && stages.count);
		if(!active) return;
		if      (type == InterpTransformType_Once){
			current += DeshTime->deltaTime;
			if(current >= duration){
				current = duration;
				active = false;
			}
			
			f32 alpha = current / duration;
			u32 lo = (u32)floor(alpha*(stages.count-1));
			u32 hi = (u32)ceil(alpha*(stages.count-1));
			physics->position = Math::lerp(stages[lo].position, stages[hi].position, alpha);
			physics->rotation = Math::lerp(stages[lo].rotation, stages[hi].rotation, alpha);
			physics->scale    = Math::lerp(stages[lo].scale,    stages[hi].scale,    alpha);
		}else if(type == InterpTransformType_Bounce){
			Assert(!"not implemented yet");
		}
	}
};

#endif //ATTRIBUTE_INTERPTRANSFORM_H