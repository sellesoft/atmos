#include "Admin.h"
#include "PhysicsSystem.h"
#include "utils/array.h"
#include "entities/Entity.h"
#include "entities/PlayerEntity.h"
#include "attributes/Physics.h"
#include "attributes/Collider.h"

struct PhysicsTuple {
	Transform* transform = 0;
	Physics*   physics	 = 0;
	Collider*  collider	 = 0;
};

array<PhysicsTuple> GetPhysicsTuples() {
	array<PhysicsTuple> out;
	for (Entity* e : AtmoAdmin->entities) 
		if (Physics* phys = e->GetAttribute<Physics>()) 
			out.add({ &e->transform, phys, e->GetAttribute<Collider>() });
	return out;
}

inline void PhysicsTick(PhysicsTuple& t, PhysicsSystem* ps) {

	//TODO contact states

	t.physics->acceleration = t.physics->netForce / t.physics->mass;
	
	//add gravity
	t.physics->acceleration += vec3(0, -ps->gravity, 0);
	
	//update linear motion
	if (!t.physics->staticPosition) {
		t.physics->velocity += t.physics->acceleration * DeshTime->fixedDeltaTime;
	
		f32 vm = t.physics->velocity.mag();
		if (vm > ps->maxVelocity) {
			t.physics->velocity /= vm;
			t.physics->velocity *= ps->maxVelocity;
		}
		else if (vm < ps->maxVelocity) {
			t.physics->velocity = vec3::ZERO;
			t.physics->acceleration = vec3::ZERO;
		}
		t.physics->position += t.physics->velocity * DeshTime->fixedDeltaTime;
	}

	//// rotation ////

	//make fake rotational friction
	if (t.physics->rotVelocity != vec3::ZERO) {
		t.physics->rotAcceleration = vec3(t.physics->rotVelocity.x > 0 ? -1 : 1, t.physics->rotVelocity.y > 0 ? -1 : 1, t.physics->rotVelocity.z > 0 ? -1 : 1) * ps->frictionAir * t.physics->mass * 100;
	}

	//update rotational motion
	t.physics->rotVelocity += t.physics->rotAcceleration * DeshTime->fixedDeltaTime;
	t.physics->rotation += t.physics->rotVelocity * DeshTime->fixedDeltaTime;

	t.physics->netForce = vec3::ZERO;
	t.physics->acceleration = 0;
}

inline void AABBAABBCollision(Physics* obj1, AABBCollider* obj1Col, Physics* obj2, AABBCollider* obj2Col) {
	vec3 min1 = obj1->position - (obj1Col->halfDims * obj1->entity->transform.scale);
	vec3 max1 = obj1->position + (obj1Col->halfDims * obj1->entity->transform.scale);
	vec3 min2 = obj2->position - (obj2Col->halfDims * obj2->entity->transform.scale);
	vec3 max2 = obj2->position + (obj2Col->halfDims * obj2->entity->transform.scale);


	if (//check if overlapping
		(min1.x <= max2.x && max1.x >= min2.x) &&
		(min1.y <= max2.y && max1.y >= min2.y) &&
		(min1.z <= max2.z && max1.z >= min2.z)) {
		
		//TODO(sushi) implement keeping track of contacts

		if (obj1Col->noCollide || obj2Col->noCollide) return;

		float xover, yover, zover;

		//we need to know which box is in front over each axis so
		//the overlap is correct
		if (max1.x < max2.x) xover = max1.x - min2.x;
		else                 xover = max2.x - min1.x;
		if (max1.y < max2.y) yover = max1.y - min2.y;
		else                 yover = max2.y - min1.y;
		if (max1.z < max2.z) zover = max1.z - min2.z;
		else                 zover = max2.z - min1.z;

		Manifold m1;
		Manifold m2;

		//static resolution
		vec3 norm;
		if (xover < yover && xover < zover) {
			if (!obj1->staticPosition) obj1->position.x += xover / 2;
			if (!obj2->staticPosition) obj2->position.x -= xover / 2;
			norm = vec3::LEFT;
		}
		else if (yover < xover && yover < zover) {
			if (!obj1->staticPosition) obj1->position.y += yover / 2;
			if (!obj2->staticPosition) obj2->position.y -= yover / 2;
			norm = vec3::DOWN;
		}
		else if (zover < yover && zover < xover) {
			if (!obj1->staticPosition) obj1->position.z += zover / 2;
			if (!obj2->staticPosition) obj2->position.z -= zover / 2;
			norm = vec3::BACK;
		}

		m1.norm = norm; m2.norm = norm;

		//dynamic resolution
		//get relative velocity between both objects with obj1 as the F.O.R
		vec3 rv = obj2->velocity - obj1->velocity;

		//find the velocity along the normal and dynamically resolve
		f32 vAlongNorm = rv.dot(norm);
		if (vAlongNorm < 0) {
			float j = -(1 + (obj1->elasticity + obj2->elasticity) / 2) * vAlongNorm;
			j /= 1 / obj1->mass + 1 / obj2->mass;

			vec3 impulse = j * norm;
			if (!obj1->staticPosition) obj1->velocity -= impulse / obj1->mass;
			if (!obj2->staticPosition) obj2->velocity += impulse / obj2->mass;
		
			//TODO(sushi) set contact state here
		
		}
	}

}

void PhysicsSystem::Init() {
	gravity = 9.81;
	frictionAir = 0.01f;
	minVelocity = 0.005f;
	maxVelocity = 100.f;
	minRotVelocity = 1.f;
	maxRotVelocity = 360.f;
}

void PhysicsSystem::Update() {

}