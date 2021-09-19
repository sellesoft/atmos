#pragma once
#ifndef ATMOS_ATTRIBUTE_PHYSICS_H
#define ATMOS_ATTRIBUTE_PHYSICS_H

#include "Attribute.h"
#include "Collider.h"
#include "math/vector.h"

struct Collider;
struct Physics{
	Attribute attribute{ AttributeType_Physics };
    Collider* collider = 0;
	
	vec3 position;
	vec3 rotation;
	vec3 scale;
	vec3 velocity;
	vec3 acceleration;
	vec3 rotVelocity;
	vec3 rotAcceleration;
    
	f32 elasticity; //less than 1 in most cases
	f32 mass;
	f32 kineticFricCoef;
	f32 staticFricCoef;
    
    b32 staticPosition = false;
	b32 staticRotation = false;
	
	vec3 netForce;
    
	Physics();
	Physics(vec3 position, vec3 rotation, vec3 velocity = vec3::ZERO, vec3 acceleration = vec3::ZERO,
            vec3 rotVeloctiy = vec3::ZERO, vec3 rotAcceleration = vec3::ZERO, float elasticity = .2f,
            float mass = 1.f, bool staticPosition = false);
	Physics(vec3 position, vec3 rotation, float mass, float elasticity);
    
    void Update(f32 alpha);
    
	//changes acceleration by adding a force to target, target also applies the force to creator
	void AddForce(vec3 force);
	//if no creator, assume air friction; if creator, assume sliding friction
	void AddFrictionForce(float frictionCoef, float grav = 9.807);
	//changes velocity by adding an impulse to target, target also applies the impulse to creator
	void AddImpulse(vec3 impulse);
	void AddImpulseNomass(vec3 impulse);
};

#endif //ATMOS_ATTRIBUTE_PHYSICS_H