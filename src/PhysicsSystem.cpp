#include "Admin.h"
#include "PhysicsSystem.h"
#include "utils/array.h"
#include "entities/Entity.h"
#include "entities/PlayerEntity.h"
#include "attributes/Physics.h"
#include "attributes/Collider.h"

bool AABBAABBCollision(Physics* p1, AABBCollider* c1, Physics* p2, AABBCollider* c2){
	vec3 min1 = (p1->position - (c1->halfDims * p1->scale)) + c1->offset;
	vec3 max1 = (p1->position + (c1->halfDims * p1->scale)) + c1->offset;
	vec3 min2 = (p2->position - (c2->halfDims * p2->scale)) + c2->offset;
	vec3 max2 = (p2->position + (c2->halfDims * p2->scale)) + c2->offset;
	
	if((min1.x <= max2.x && max1.x >= min2.x) && //check if overlapping
	   (min1.y <= max2.y && max1.y >= min2.y) &&
	   (min1.z <= max2.z && max1.z >= min2.z)){
		
		//TODO(sushi) implement keeping track of contacts
		
		//early out if no collision or both are static
		if(c1->noCollide || c2->noCollide) return true;
		if(p1->staticPosition && p2->staticPosition) return true;
		
		vec3 norm;
		
		//// static resolution ////
		f32 xover = (max1.x < max2.x) ? max1.x - min2.x : max2.x - min1.x; //we need to know which box is in front 
		f32 yover = (max1.y < max2.y) ? max1.y - min2.y : max2.y - min1.y; //over each axis so the overlap is correct
		f32 zover = (max1.z < max2.z) ? max1.z - min2.z : max2.z - min1.z;
		if      (xover < yover && xover < zover){
			if     (p1->staticPosition){ p2->position.x -= xover; }
			else if(p2->staticPosition){ p1->position.x += xover; }
			else                       { p1->position.x += xover/2.f; p2->position.x -= xover/2.f; }
			norm = vec3::LEFT;
		}else if(yover < xover && yover < zover){
			if     (p1->staticPosition){ p2->position.y -= yover; }
			else if(p2->staticPosition){ p1->position.y += yover; }
			else                       { p1->position.y += yover/2.f; p2->position.y -= yover/2.f; }
			norm = vec3::DOWN;
		}else if(zover < yover && zover < xover){
			if     (p1->staticPosition){ p2->position.z -= zover; }
			else if(p2->staticPosition){ p1->position.z += zover; }
			else                       { p1->position.z += zover/2.f; p2->position.z -= zover/2.f; }
			norm = vec3::BACK;
		}
		
		//TODO(sushi) create manifolds
		
		//// dynamic resolution ////
		//get relative velocity between both objects with p1 as the F.O.R
		vec3 rv = p2->velocity - p1->velocity;
		
		//find the velocity along the normal and dynamically resolve
		f32 vAlongNorm = rv.dot(norm);
		if(vAlongNorm < 0){
			float j = -(1 + (p1->elasticity + p2->elasticity) / 2) * vAlongNorm;
			j /= 1.f/p1->mass + 1.f/p2->mass;
			
			vec3 impulse = j * norm;
			p1->velocity -= impulse / p1->mass;
			p2->velocity += impulse / p2->mass;
			
			//TODO(sushi) set contact state here
		}
		return true;
	}else{
		return false;
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
		AtmoAdmin->player->Update();
		for(Physics* p1 = AtmoAdmin->physicsArr.begin(); p1 != AtmoAdmin->physicsArr.end(); ++p1){
			if(p1->attribute.entity != AtmoAdmin->player){
				//TODO contact states
				
				//linear motion
				if(!p1->staticPosition){
					p1->acceleration += vec3(0, -gravity, 0); //add gravity
					p1->velocity += p1->acceleration * fixedDeltaTime;
					
					f32 vm = p1->velocity.mag();
					if(vm > maxVelocity) {
						p1->velocity /= vm;
						p1->velocity *= maxVelocity;
					}else if(vm < minVelocity){
						p1->velocity = vec3::ZERO;
						p1->acceleration = vec3::ZERO;
					}
					p1->position += p1->velocity * fixedDeltaTime;
					
					p1->acceleration = vec3::ZERO;
				}
				
				//rotational motion
				if(!p1->staticRotation){
					if(p1->rotVelocity != vec3::ZERO){ //fake rotational friction
						p1->rotAcceleration += vec3(p1->rotVelocity.x > 0 ? -1 : 1, 
													p1->rotVelocity.y > 0 ? -1 : 1, 
													p1->rotVelocity.z > 0 ? -1 : 1) * p1->airFricCoef * p1->mass * 100;
					}
					
					p1->rotVelocity += p1->rotAcceleration * fixedDeltaTime;
					p1->rotation += p1->rotVelocity * fixedDeltaTime;
				}
			}
			
			//collision check
			if(p1->collider && p1->collider->type != ColliderType_NONE){
				for(Physics* p2 = AtmoAdmin->physicsArr.begin(); p2 != AtmoAdmin->physicsArr.end(); ++p2){
					//check if able to collide
					if((p1 != p2) && (p2->collider != 0) && (p2->collider->type != ColliderType_NONE) 
					   && (p1->collider->layer == p2->collider->layer)){
						//find collision type and check for it
						bool collision = false;
						switch(p1->collider->type){
							case ColliderType_AABB:
							switch(p2->collider->type){
								case ColliderType_AABB:{ collision = AABBAABBCollision(p1, (AABBCollider*)p1->collider, 
																					   p2, (AABBCollider*)p2->collider); }break;
								default: Assert(!"not implemented"); break;
							}break;
							default: Assert(!"not implemented"); break;
						}
						//set triggers as active
						if(collision){
							if(p1->collider->isTrigger) p1->collider->triggerActive = true;
							if(p2->collider->isTrigger) p2->collider->triggerActive = true;
							//Log("physics","Collision between '",p1->attribute.entity->name,"' and '",p2->attribute.entity->name,"'");
						}
					}
				}
				//TODO fill manifolds
				//TODO solve manifolds
			}
		}
		AtmoAdmin->player->PostCollisionUpdate();
		
		//// update fixed time ////
		fixedAccumulator -= fixedDeltaTime;
		fixedTotalTime += fixedDeltaTime;
	}
	fixedAlpha = fixedAccumulator / fixedDeltaTime;
}