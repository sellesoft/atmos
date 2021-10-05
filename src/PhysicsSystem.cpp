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

void AABBAABBCollision(Physics* p0, AABBCollider* c0, Physics* p1, AABBCollider* c1, Manifold* manifold = 0){
	vec3 min0 = (p0->position - (c0->halfDims * p0->scale)) + c0->offset;
	vec3 max0 = (p0->position + (c0->halfDims * p0->scale)) + c0->offset;
	vec3 min1 = (p1->position - (c1->halfDims * p1->scale)) + c1->offset;
	vec3 max1 = (p1->position + (c1->halfDims * p1->scale)) + c1->offset;
	
	if((min0.x <= max1.x && max0.x >= min1.x) && //check if overlapping
	   (min0.y <= max1.y && max0.y >= min1.y) &&
	   (min0.z <= max1.z && max0.z >= min1.z)){
		if(manifold) manifold->state = ContactState_Stationary;
		
		//early out if no collision or both are static
		if(c0->noCollide || c1->noCollide) return;
		if(p0->staticPosition && p1->staticPosition) return;
		
		//// static resolution ////
		vec3 normal = vec3::ZERO;
		f32 xover = (max0.x < max1.x) ? max0.x - min1.x : max1.x - min0.x; //we need to know which box is in front 
		f32 yover = (max0.y < max1.y) ? max0.y - min1.y : max1.y - min0.y; //over each axis so the overlap is correct
		f32 zover = (max0.z < max1.z) ? max0.z - min1.z : max1.z - min0.z;
		if      (xover < yover && xover < zover){
			if     (p0->staticPosition){ p1->position.x -= xover; }
			else if(p1->staticPosition){ p0->position.x += xover; }
			else                       { p0->position.x += xover/2.f; p1->position.x -= xover/2.f; }
			normal = vec3::LEFT;
		}else if(yover < xover && yover < zover){
			if     (p0->staticPosition){ p1->position.y -= yover; }
			else if(p1->staticPosition){ p0->position.y += yover; }
			else                       { p0->position.y += yover/2.f; p1->position.y -= yover/2.f; }
			normal = vec3::DOWN;
		}else if(zover < yover && zover < xover){
			if     (p0->staticPosition){ p1->position.z -= zover; }
			else if(p1->staticPosition){ p0->position.z += zover; }
			else                       { p0->position.z += zover/2.f; p1->position.z -= zover/2.f; }
			normal = vec3::BACK;
		}
		if(manifold) manifold->normal = normal;
		
		//// dynamic resolution ////
		f32 vAlongNorm = normal.dot(p1->velocity - p0->velocity); //relative velocity along the normal with p0 as the F.O.R
		if(vAlongNorm < 0){
			float j = -(1.f + ((p0->elasticity + p1->elasticity) / 2.f)) * vAlongNorm;
			j /= (1.f/p0->mass + 1.f/p1->mass);
			
			vec3 impulse = normal * j;
			p0->velocity -= impulse / p0->mass;
			p1->velocity += impulse / p1->mass;
			
			if(manifold && ((!p0->staticPosition && fabs(p0->velocity.normalized().dot(normal)) != 1) ||
							(!p1->staticPosition && fabs(p1->velocity.normalized().dot(normal)) != 1))){
				manifold->state = ContactState_Moving;
			}
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
			if(it->attribute.entity != AtmoAdmin->player){
				//TODO contact state frictions
				
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
		}
		
		//// broad collision detection //// (filter manifolds)
		array<Manifold> manifolds;
		for(Physics* p0 = AtmoAdmin->physicsArr.begin(); p0 != AtmoAdmin->physicsArr.end(); ++p0){
			if(p0->collider && p0->collider->type != ColliderType_NONE){
				for(Physics* p1 = p0+1; p1 != AtmoAdmin->physicsArr.end(); ++p1){
					if(    (p1->collider) 
					   &&  (p1->collider->type != ColliderType_NONE) 
					   &&  (p0->collider->layer == p1->collider->layer) 
					   && !(p0->collider->playerOnly && p1->attribute.entity != AtmoAdmin->player) 
					   && !(p1->collider->playerOnly && p0->attribute.entity != AtmoAdmin->player)){
						manifolds.add(Manifold{p0->attribute.entity, p1->attribute.entity, p0, p1, p0->collider, p1->collider});
					}
				}
			}
		}
		
		//// narrow collision detection //// (fill and solve manifolds)
		forE(manifolds){
			switch((1 << it->c0->type) | (1 << it->c1->type)){
				case CollisionType_AABBAABB:{ AABBAABBCollision(it->p0, (AABBCollider*)it->c0, it->p1, (AABBCollider*)it->c1, it); }break;
				default: Assert(!"not implemented"); break;
			}
			
			//set triggers as active
			if(it->state != ContactState_NONE){
				if(it->c0->isTrigger) it->p0->collider->triggerActive = true;
				if(it->c1->isTrigger) it->p1->collider->triggerActive = true;
				Log("physics","Collision between '",it->e0->name,"' and '",it->e1->name,"'");
			}
		}
		
		AtmoAdmin->player->PostCollisionUpdate();
		
		//// update fixed time ////
		fixedAccumulator -= fixedDeltaTime;
		fixedTotalTime += fixedDeltaTime;
	}
	fixedAlpha = fixedAccumulator / fixedDeltaTime;
}