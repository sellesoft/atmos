#include "Physics.h"
#include "../admin.h"
#include "../entities/entity.h"
#include "utils/string_conversion.h"
#include "core/storage.h"

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
	acceleration += force / mass;
}

void Physics::AddFrictionForce(float frictionCoef, float grav) {
	acceleration += (-velocity.normalized() * frictionCoef * mass * grav) / mass;
}

void Physics::AddImpulse(vec3 impulse) {
	velocity += impulse / mass;
}

void Physics::AddImpulseNomass(vec3 impulse) {
	velocity += impulse;
}

void Physics::SaveText(Physics* p, string& level){
	level += TOSTRING("\n:",AttributeType_Physics," #",AttributeTypeStrings[AttributeType_Physics],
					  "\nposition     ",p->position,
					  "\nrotation     ",p->rotation,
					  "\nscale        ",p->scale,
					  "\nvelocity     ",p->velocity,
					  "\naccel        ",p->acceleration,
					  "\nrot_velocity ",p->rotVelocity,
					  "\nrot_accel    ",p->rotAcceleration,
					  "\nmass         ",p->mass,
					  "\nelasticity   ",p->elasticity,
					  "\nkinetic_fric ",p->kineticFricCoef,
					  "\nstatic_fric  ",p->staticFricCoef,
					  "\nair_fric     ",p->airFricCoef,
					  "\nstatic_pos   ",(p->staticPosition) ? "true" : "false",
					  "\nstatic_rot   ",(p->staticRotation) ? "true" : "false");
	if(p->collider.type != ColliderType_NONE){
		level += TOSTRING("\ncollider_type       ",p->collider.type," #",ColliderTypeStrings[p->collider.type],
						  "\ncollider_offset     ",p->collider.offset,
						  "\ncollider_layer      ",p->collider.layer,
						  "\ncollider_nocollide  ",(p->collider.noCollide) ? "true" : "false",
						  "\ncollider_trigger    ",(p->collider.isTrigger) ? "true" : "false",
						  "\ncollider_playeronly ",(p->collider.playerOnly) ? "true" : "false");
		switch(p->collider.type){
			case ColliderType_AABB:{
				level += TOSTRING("\ncollider_half_dims ",p->collider.halfDims);
			}break;
			case ColliderType_Sphere:{
				level += TOSTRING("\ncollider_radius    ",p->collider.radius);
			}break;
			case ColliderType_Hull:{
				Storage::SaveMesh(p->collider.mesh);
				level += TOSTRING("\ncollider_mesh      \"",p->collider.mesh->name,"\"");
			}break;
		}
	}
}