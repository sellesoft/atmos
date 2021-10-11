#include "Admin.h"
#include "PhysicsSystem.h"
#include "utils/array.h"
#include "entities/Entity.h"
#include "entities/PlayerEntity.h"
#include "attributes/Physics.h"
#include "attributes/Collider.h"

//debug
#include "core/renderer.h"

enum CollisionType{
	CollisionType_AABBAABB     = (1<<ColliderType_AABB)  |(1<<ColliderType_AABB),
	CollisionType_AABBSphere   = (1<<ColliderType_AABB)  |(1<<ColliderType_Sphere),
	CollisionType_SphereSphere = (1<<ColliderType_Sphere)|(1<<ColliderType_Sphere),
};

FORCE_INLINE bool AABBAABBTest(vec3 min0, vec3 max0, vec3 min1, vec3 max1){
	return (min0.x <= max1.x && max0.x >= min1.x) && (min0.y <= max1.y && max0.y >= min1.y) && (min0.z <= max1.z && max0.z >= min1.z);
}

void AABBAABBCollision(Physics* p0, Collider* c0, Physics* p1, Collider* c1, Manifold* manifold){
	vec3 min0 = (p0->position - (c0->halfDims * p0->scale)) + c0->offset;
	vec3 max0 = (p0->position + (c0->halfDims * p0->scale)) + c0->offset;
	vec3 min1 = (p1->position - (c1->halfDims * p1->scale)) + c1->offset;
	vec3 max1 = (p1->position + (c1->halfDims * p1->scale)) + c1->offset;
	
	if(AABBAABBTest(min0, max0, min1, max1)){
		manifold->state = ContactState_Stationary;
		
		//early out if no collision or both are static
		if(c0->noCollide || c1->noCollide) return;
		if(p0->staticPosition && p1->staticPosition) return;
		
		//// manifold generation ////
		persist const vec3 normals[6] = { 
			vec3::LEFT, vec3::RIGHT, 
			vec3::DOWN, vec3::UP, 
			vec3::BACK, vec3::FORWARD 
		};
		f32 distances[6] = { 
			max1.x - min0.x, max0.x - min1.x,
			max1.y - min0.y, max0.y - min1.y,
			max1.z - min0.z, max0.z - min1.z,
		};
		f32 penetration = FLT_MAX;
		vec3 normal;
		
		forI(6){
			if(distances[i] < penetration){
				penetration = distances[i]; 
				normal = normals[i];
			}
		}
		manifold->contactCount = 1;
		manifold->contacts[0].local0 = vec3::ZERO;
		manifold->contacts[0].local1 = vec3::ZERO;
		manifold->contacts[0].normal = normal;
		manifold->contacts[0].penetration = penetration;
		
		//// static resolution ////
		if     (p0->staticPosition){ p1->position += normal*penetration; }
		else if(p1->staticPosition){ p0->position -= normal*penetration; }
		else                       { p0->position -= normal*penetration/2.f; p1->position += normal*penetration/2.f; }
		
		//// dynamic resolution ////
		f32 vAlongNorm = normal.dot(p1->velocity - p0->velocity); //relative velocity along the normal with p0 as the F.O.R
		if(vAlongNorm < 0){
			float j = -(1.f + ((p0->elasticity + p1->elasticity) / 2.f)) * vAlongNorm;
			j /= (1.f/p0->mass + 1.f/p1->mass);
			
			vec3 impulse = normal * j;
			p0->velocity -= impulse / p0->mass;
			p1->velocity += impulse / p1->mass;
			
			if(((!p0->staticPosition && fabs(p0->velocity.normalized().dot(normal)) != 1) ||
				(!p1->staticPosition && fabs(p1->velocity.normalized().dot(normal)) != 1))){
				manifold->state = ContactState_Moving;
			}
		}
	}
}

void AABBSphereCollision(Physics* p0, Collider* c0, Physics* p1, Collider* c1, Manifold* manifold){
	
}

void SphereSphereCollision(Physics* p0, Collider* c0, Physics* p1, Collider* c1, Manifold* manifold){
	f32  radii = c0->radius + c1->radius;
	vec3 delta = p1->position - p0->position;
	f32  dist  = delta.mag();
	
	if(dist < radii){
		manifold->state = ContactState_Stationary;
		
		//early out if no collision or both are static
		if(c0->noCollide || c1->noCollide) return;
		if(p0->staticPosition && p1->staticPosition) return;
		
		//// manifold generation ////
		f32 penetration = radii - dist;
		vec3 normal = delta / dist;
		manifold->contactCount = 1;
		manifold->contacts[0].local0 = normal * c0->radius;
		manifold->contacts[0].local1 = -normal * c1->radius;
		manifold->contacts[0].normal = normal;
		manifold->contacts[0].penetration = penetration;
		
		//// static resolution ////
		if     (p0->staticPosition){ p1->position += normal*penetration; }
		else if(p1->staticPosition){ p0->position -= normal*penetration; }
		else                       { p0->position -= normal*penetration/2.f; p1->position += normal*penetration/2.f; }
		
		//// dynamic resolution //// //TODO rotation and elasticity
		f32 vAlongNorm = normal.dot(p1->velocity - p0->velocity); //relative velocity along the normal with p0 as the F.O.R
		f32 j = (2.f*(vAlongNorm)) / (p0->mass + p1->mass);
		p0->velocity += normal * j * p1->mass;
		p1->velocity -= normal * j * p0->mass;
		
		if(((!p0->staticPosition && fabs(p0->velocity.normalized().dot(normal)) != 1) ||
			(!p1->staticPosition && fabs(p1->velocity.normalized().dot(normal)) != 1))){
			manifold->state = ContactState_Moving;
		}
	}
}

void PhysicsSystem::Init(f32 fixedUpdatesPerSecond){
	gravity        = 9.81f;
	minVelocity    = 0.005f;
	maxVelocity    = 100.f;
	minRotVelocity = 1.f;
	maxRotVelocity = 360.f;
	
	fixedTimeStep    = fixedUpdatesPerSecond;
	fixedDeltaTime   = 1.f / fixedUpdatesPerSecond;
	fixedTotalTime   = 0;
	fixedUpdateCount = 0;
	fixedAccumulator = 0;
	
	paused = false;
}

void PhysicsSystem::Update(){
	if(paused) return;
	
	fixedAccumulator += DeshTime->deltaTime;
	while(fixedAccumulator >= fixedDeltaTime){
		//// integration ////
		AtmoAdmin->player->Update();
		forE(AtmoAdmin->physicsArr){
			if(it->attribute.entity == AtmoAdmin->player) continue;
			
			//linear motion
			if(!it->staticPosition){
				it->acceleration += vec3(0, -gravity, 0); //add gravity
				it->velocity += it->acceleration * fixedDeltaTime;
				
				f32 vm = it->velocity.mag();
				if(vm > maxVelocity) {
					it->velocity /= vm;
					it->velocity *= maxVelocity;
				}else if(vm < minVelocity){
					it->velocity = vec3::ZERO;
					it->acceleration = vec3::ZERO;
				}
				it->position += it->velocity * fixedDeltaTime;
				
				it->acceleration = vec3::ZERO;
			}
			
			//rotational motion
			if(!it->staticRotation){
				if(it->rotVelocity != vec3::ZERO){ //fake rotational friction
					it->rotAcceleration += vec3(it->rotVelocity.x > 0 ? -1 : 1, 
												it->rotVelocity.y > 0 ? -1 : 1, 
												it->rotVelocity.z > 0 ? -1 : 1) * it->airFricCoef * it->mass * 100;
				}
				
				it->rotVelocity += it->rotAcceleration * fixedDeltaTime;
				it->rotation += it->rotVelocity * fixedDeltaTime;
			}
		}
		
		//// broad collision detection //// (filter manifolds)
		array<Manifold> manifolds;
		for(Physics* p0 = AtmoAdmin->physicsArr.begin(); p0 != AtmoAdmin->physicsArr.end(); ++p0){
			if(p0->collider.type != ColliderType_NONE){
				for(Physics* p1 = p0+1; p1 != AtmoAdmin->physicsArr.end(); ++p1){
					if(    (p1->collider.type != ColliderType_NONE) 
					   &&  (p0->collider.layer == p1->collider.layer) 
					   && !(p0->collider.playerOnly && p1->attribute.entity != AtmoAdmin->player) 
					   && !(p1->collider.playerOnly && p0->attribute.entity != AtmoAdmin->player)){
						manifolds.add(Manifold{p0->attribute.entity, p1->attribute.entity, p0, p1, &p0->collider, &p1->collider});
					}
				}
			}
		}
		
		//// narrow collision detection //// (fill manifolds)
		forE(manifolds){
			switch((1 << it->c0->type) | (1 << it->c1->type)){
				case CollisionType_AABBAABB:    { AABBAABBCollision    (it->p0, it->c0, it->p1, it->c1, it); }break;
				case CollisionType_SphereSphere:{ SphereSphereCollision(it->p0, it->c0, it->p1, it->c1, it); }break;
				default: Assert(!"not implemented"); break;
			}
			
			//set triggers as active
			if(it->state != ContactState_NONE){
				if(it->c0->isTrigger) it->c0->triggerActive = true;
				if(it->c1->isTrigger) it->c1->triggerActive = true;
				//Log("physics","Collision between '",it->e0->name,"' and '",it->e1->name,"'");
			}
		}
		
		//// collision resolution //// (solve manifolds)
		/*
		forE(manifolds){
			if(it->type == ContactState_NONE) continue;
			
			forI(it->contactCount){
				vec3 normal = it->contacts[i].normal;
				f32  penetration = it->contacts[i].penetration;
				
				//static resolution
				if     (p0->staticPosition){ p1->position += normal*penetration; }
				else if(p1->staticPosition){ p0->position -= normal*penetration; }
				else{ p0->position -= normal*penetration/2.f; p1->position += normal*penetration/2.f; }
				
				//dynamic resolution
				vec2 r0 = it->p0->position + it->contacts[i].local0;
				vec2 r1 = it->p1->position + it->contacts[i].local1;
				
			}
		}
		*/
		AtmoAdmin->player->PostCollisionUpdate();
		
		//// update fixed time ////
		fixedAccumulator -= fixedDeltaTime;
		fixedTotalTime += fixedDeltaTime;
	}
	fixedAlpha = fixedAccumulator / fixedDeltaTime;
}