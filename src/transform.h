#pragma once
#ifndef ATMOS_TRANSFORM_H
#define ATMOS_TRANSFORM_H

#include "math/VectorMatrix.h"

struct Transform{
	vec3 position = vec3::ZERO;
	vec3 rotation = vec3::ZERO;
	vec3 scale    = vec3::ONE;
    
	Transform(vec3 _position = vec3::ZERO, vec3 _rotation = vec3::ZERO, vec3 _scale = vec3::ONE){
		position = _position; rotation = _rotation; scale = _scale;
	}
	
	inline vec3 Up()     { return vec3::UP      * mat4::RotationMatrix(rotation); }
	inline vec3 Right()  { return vec3::RIGHT   * mat4::RotationMatrix(rotation); }
	inline vec3 Forward(){ return vec3::FORWARD * mat4::RotationMatrix(rotation); }
	inline mat4 Matrix() { return mat4::TransformationMatrix(position, rotation, scale); }
};

#endif //ATMOS_TRANSFORM_H
