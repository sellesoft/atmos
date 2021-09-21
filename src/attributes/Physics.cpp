#include "Physics.h"
#include "../admin.h"
#include "../entities/entity.h"

Physics::Physics() {
	position = vec3::ZERO;
	rotation = vec3::ZERO;
	velocity = vec3::ZERO;
	acceleration = vec3::ZERO;
	rotVelocity = vec3::ZERO;
	rotAcceleration = vec3::ZERO;
	elasticity = .2f;
	mass = 1;
	staticPosition = false;
}

Physics::Physics(vec3 position, vec3 rotation, vec3 velocity, vec3 acceleration, vec3 rotVeloctiy,
                 vec3 rotAcceleration, float elasticity, float mass, bool staticPosition) {
	this->position = position;
	this->rotation = rotation;
	this->velocity = velocity;
	this->acceleration = acceleration;
	this->rotVelocity = rotVeloctiy;
	this->rotAcceleration = rotAcceleration;
	this->elasticity = elasticity;
	this->mass = mass;
	this->staticPosition = staticPosition;
}

Physics::Physics(vec3 position, vec3 rotation, float mass, float elasticity) {
	this->position = position;
	this->rotation = rotation;
	this->velocity = vec3::ZERO;
	this->acceleration = vec3::ZERO;
	this->rotVelocity = vec3::ZERO;
	this->rotAcceleration = vec3::ZERO;
	this->mass = mass;
	this->elasticity = elasticity;
}

void Physics::Update(f32 alpha){
    //interpolate between new physics transform and old transform by the leftover time
    attribute.entity->transform.position = (attribute.entity->transform.position * (1.f - alpha)) + (position * alpha);
    attribute.entity->transform.rotation = (attribute.entity->transform.rotation * (1.f - alpha)) + (rotation * alpha);
    attribute.entity->transform.scale    = (attribute.entity->transform.scale    * (1.f - alpha)) + (scale    * alpha);
}

void Physics::AddForce(vec3 force) {
	netForce += force;
}

void Physics::AddFrictionForce(float frictionCoef, float grav) {
	netForce += -velocity.normalized() * frictionCoef * mass * grav;
}

void Physics::AddImpulse(vec3 impulse) {
	velocity += impulse / mass;
}

void Physics::AddImpulseNomass(vec3 impulse) {
	velocity += impulse;
}