#pragma once
#ifndef ATTRIBUTE_PHYSICS_H
#define ATTRIBUTE_PHYSICS_H

#include "Attribute.h"
#include "math/Vector.h"
#include "utils/map.h"


typedef u32 ColliderType;
struct Physics;
struct Collider;

struct Manifold {
	Collider* a = nullptr;
	Collider* b = nullptr;

	ColliderType coltypea;
	ColliderType coltypeb;

	int refID = 0;
	//point then depth
	std::vector<pair<vec3, float>> colpoints;

	vec3 norm;
};

enum ContactState {
	ContactNONE,
	ContactStationary,
	ContactMoving
};

struct Physics {
	Attribute attribute{ AttributeType_Physics };
	
	vec3 position;
	vec3 rotation;
	vec3 scale;

	vec3 velocity;
	vec3 acceleration;
	vec3 rotVelocity;
	vec3 rotAcceleration;

	float elasticity; //less than 1 in most cases
	float mass;

	vec3 netForce;

	bool staticPosition = false;
	bool staticRotation = false;
	
	float kineticFricCoef;
	float staticFricCoef;
	
	ContactState contactState;
	map<Physics*, ContactState> contacts;

	Physics();
	Physics(vec3 position, vec3 rotation, vec3 velocity = vec3::ZERO, vec3 acceleration = vec3::ZERO,
		vec3 rotVeloctiy = vec3::ZERO, vec3 rotAcceleration = vec3::ZERO, float elasticity = .2f,
		float mass = 1.f, bool staticPosition = false);
	Physics(vec3 position, vec3 rotation, float mass, float elasticity);

	//changes acceleration by adding a force to target, target also applies the impulse to creator
	void AddForce(vec3 force);

	//if no creator, assume air friction; if creator, assume sliding friction
	void AddFrictionForce(float frictionCoef, float grav = 9.807);

	//changes velocity by adding an impulse to target, target also applies the impulse to creator
	void AddImpulse(vec3 impulse);
	void AddImpulseNomass(vec3 impulse);
};

#endif 