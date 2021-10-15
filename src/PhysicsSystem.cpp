#include "Admin.h"
#include "PhysicsSystem.h"
#include "utils/array.h"
#include "entities/Entity.h"
#include "entities/PlayerEntity.h"
#include "attributes/Physics.h"
#include "attributes/Collider.h"
#include "geometry/geometry.h"

//debug
#include "core/renderer.h"

//////////////////
//// @utility ////
//////////////////
#define PHYSICS_AABB_FACE_COUNT 6
local const vec3 physics_aabb_normals[PHYSICS_AABB_FACE_COUNT] = { 
	vec3::RIGHT, vec3::UP,   vec3::FORWARD,
	vec3::LEFT,  vec3::DOWN, vec3::BACK,
};
#define PHYSICS_MAX_COLLISION_AXIS_COUNT 1024
local u32  physics_possible_collision_count = 0;
local vec3 physics_possible_collision_axes[PHYSICS_MAX_COLLISION_AXIS_COUNT];

FORCE_INLINE bool AABBAABBTest(vec3 min0, vec3 max0, vec3 min1, vec3 max1){
	return (min0.x <= max1.x && max0.x >= min1.x) && (min0.y <= max1.y && max0.y >= min1.y) && (min0.z <= max1.z && max0.z >= min1.z);
}

FORCE_INLINE local void AddPossibleCollisionAxis(vec3 axis){
	Assert(physics_possible_collision_count < PHYSICS_MAX_COLLISION_AXIS_COUNT);
	forI(physics_possible_collision_count){ if(physics_possible_collision_axes[i] == axis){ return; } }
	physics_possible_collision_axes[physics_possible_collision_count++] = axis;
}

FORCE_INLINE local void ResetPossibleCollisionAxes(){
	physics_possible_collision_count = 0;
}

////////////////////
//// @detection ////
////////////////////
void AABBAABBCollision(Physics* p0, Collider* c0, Physics* p1, Collider* c1, Manifold* m){
	vec3 min0 = (p0->position - (c0->halfDims * p0->scale)) + c0->offset;
	vec3 max0 = (p0->position + (c0->halfDims * p0->scale)) + c0->offset;
	vec3 min1 = (p1->position - (c1->halfDims * p1->scale)) + c1->offset;
	vec3 max1 = (p1->position + (c1->halfDims * p1->scale)) + c1->offset;
	
	if(AABBAABBTest(min0, max0, min1, max1)){
		//early out if no collision or both are static
		m->state = ContactState_Stationary;
		if(c0->noCollide || c1->noCollide) return;
		if(p0->staticPosition && p1->staticPosition && p0->staticRotation && p1->staticRotation) return;
		
		f32 distances[PHYSICS_AABB_FACE_COUNT] = { 
			max0.x - min1.x, max0.y - min1.y, max0.z - min1.z,
			max1.x - min0.x, max1.y - min0.y, max1.z - min0.z, 
		};
		f32 penetration = FLT_MAX;
		vec3 normal;
		
		forI(PHYSICS_AABB_FACE_COUNT){
			if(distances[i] < penetration){
				penetration = distances[i]; 
				normal = physics_aabb_normals[i];
			}
		}
		m->contactCount = 1;
		m->contacts[0].local0 = vec3::ZERO;
		m->contacts[0].local1 = vec3::ZERO;
		m->contacts[0].normal = normal;
		m->contacts[0].penetration = penetration;
	}
}

void AABBSphereCollision(Physics* p0, Collider* c0, Physics* p1, Collider* c1, Manifold* m){
	vec3 aabb_point = Geometry::ClosestPointOnAABB(p0->position, (c0->halfDims*p0->scale), p1->position);
	vec3 delta = aabb_point - p1->position;
	f32  distance = delta.mag();
	
	if(distance < c1->radius){
		//early out if no collision or both are static
		m->state = ContactState_Stationary;
		if(c0->noCollide || c1->noCollide) return;
		if(p0->staticPosition && p1->staticPosition && p0->staticRotation && p1->staticRotation) return;
		
		f32 penetration = c1->radius - distance;
		vec3 normal = delta / distance;
		m->contactCount = 1;
		m->contacts[0].local0 = vec3::ZERO;
		m->contacts[0].local1 = -normal * c1->radius;
		m->contacts[0].normal = normal;
		m->contacts[0].penetration = penetration;
	}
}

void AABBConvexMeshCollision(Physics* p0, Collider* c0, Physics* p1, Collider* c1, Manifold* m){
	ResetPossibleCollisionAxes();
	
	mat4 aabb_transform = mat4::TransformationMatrix(p0->position, vec3::ZERO, p0->scale);
	mat4 mesh_transform = mat4::TransformationMatrix(p1->position, p1->rotation, p1->scale); 
	mat4 mesh_to_world_to_aabb_space = mesh_transform * aabb_transform.Inverse();
	
	const f32 aabb_min_vertexes[PHYSICS_AABB_FACE_COUNT/2] = { -c0->halfDims.x, -c0->halfDims.y, -c0->halfDims.z };
	const f32 aabb_max_vertexes[PHYSICS_AABB_FACE_COUNT/2] = {  c0->halfDims.x,  c0->halfDims.y,  c0->halfDims.z };
	
	//collect possible collision axes
	forX(aabb_normal_idx, PHYSICS_AABB_FACE_COUNT/2){
		AddPossibleCollisionAxis(physics_aabb_normals[aabb_normal_idx]);
		for(MeshFace& face : c1->mesh->faces){
			vec3 prev_pos = c1->mesh->vertexes[face.outerVertexes[0]].pos;
			for(u32 vert_idx = 1; vert_idx < face.outerVertexCount; ++vert_idx){
				vec3 edge = ((c1->mesh->vertexes[face.outerVertexes[vert_idx]].pos - prev_pos) * mesh_to_world_to_aabb_space).normalized();
				AddPossibleCollisionAxis(physics_aabb_normals[aabb_normal_idx].cross(edge));
				prev_pos = c1->mesh->vertexes[face.outerVertexes[vert_idx]].pos;
			}
		}
	}
	forE(c1->mesh->faces) AddPossibleCollisionAxis(it->normal);
	
	//check if the objects overlap after projection onto the possible collision axes
	m->contacts[0].penetration = -FLT_MAX;
	forX(axis_idx, physics_possible_collision_count){
		vec3 axis = physics_possible_collision_axes[axis_idx];
		
		//find min and max aabb vertexes projected on the axis
		f32 aabb_min_vertex = axis.dot(c0->halfDims);
		f32 aabb_max_vertex = axis.dot(-c0->halfDims);
		
		//find min and max mesh vertexes projected on the axis
		f32 mesh_min_vertex = FLT_MAX;
		f32 mesh_max_vertex = -FLT_MAX;
		for(MeshFace& face : c1->mesh->faces){
			for(u32 vert_idx : face.outerVertexes){
				f32 mesh_local_vertex = axis.dot(c1->mesh->vertexes[vert_idx].pos * mesh_to_world_to_aabb_space);
				if(mesh_local_vertex < mesh_min_vertex) mesh_min_vertex = mesh_local_vertex;
				if(mesh_local_vertex > mesh_max_vertex) mesh_max_vertex = mesh_local_vertex;
			}
		}
		
		//check if mesh vertexes overlap aabb vertexes on the axis
		if(aabb_min_vertex <= mesh_min_vertex && aabb_max_vertex >= mesh_min_vertex){
			f32 local_penetration = mesh_min_vertex - aabb_max_vertex;
			if(local_penetration >= m->contacts[0].penetration){
				m->state = ContactState_Stationary;
				m->contacts[0].local0 = c0->halfDims + (axis * local_penetration);
				m->contacts[0].normal = axis;
				m->contacts[0].penetration = local_penetration;
			}
			continue; //collision on this axis
		}
		if(mesh_min_vertex <= aabb_min_vertex && mesh_max_vertex >= aabb_min_vertex){
			f32 local_penetration = aabb_min_vertex - mesh_max_vertex;
			if(local_penetration >= m->contacts[0].penetration){
				m->state = ContactState_Stationary;
				m->contacts[0].local0 = -c0->halfDims + (-axis * local_penetration);
				m->contacts[0].normal = -axis;
				m->contacts[0].penetration = local_penetration;
			}
			continue; //collision on this axis in the negative direction
		}
		m->state = ContactState_NONE;
		return; //no collision on this axis
	}
	
	//Render::DrawLine();
	//TODO find contact points
}

void SphereSphereCollision(Physics* p0, Collider* c0, Physics* p1, Collider* c1, Manifold* m){
	f32  radii = c0->radius + c1->radius;
	vec3 delta = p1->position - p0->position;
	f32  distance = delta.mag();
	
	if(distance < radii){
		//early out if no collision or both are static
		m->state = ContactState_Stationary;
		if(c0->noCollide || c1->noCollide) return;
		if(p0->staticPosition && p1->staticPosition && p0->staticRotation && p1->staticRotation) return;
		
		f32 penetration = radii - distance;
		vec3 normal = delta / distance;
		m->contactCount = 1;
		m->contacts[0].local0 = normal * c0->radius;
		m->contacts[0].local1 = -normal * c1->radius;
		m->contacts[0].normal = normal;
		m->contacts[0].penetration = penetration;
	}
}

void SphereConvexMeshCollision(Physics* p0, Collider* c0, Physics* p1, Collider* c1, Manifold* m){
	vec3 mesh_point = Geometry::ClosestPointOnConvexMesh(c1->mesh, p1->position, p1->rotation, p1->scale, p0->position);
	vec3 delta = mesh_point - p0->position;
	f32  distance = delta.mag();
	
#if 0
	Render::DrawLine(p0->position, p1->position, Color_Blue);
	Render::DrawBox(mat4::TransformationMatrix(mesh_point, vec3::ZERO, vec3(.1f,.1f,.1f)), Color_Red);
#endif
	
	if(distance < c0->radius){
		//early out if no collision or both are static
		m->state = ContactState_Stationary;
		if(c0->noCollide || c1->noCollide) return;
		if(p0->staticPosition && p1->staticPosition && p0->staticRotation && p1->staticRotation) return;
		
		f32 penetration = c0->radius - distance;
		vec3 normal = (mesh_point - p1->position).normalized();
		m->contactCount = 1;
		m->contacts[0].local0 = (delta / distance) * c0->radius;
		m->contacts[0].local1 = mesh_point * mat4::TransformationMatrix(p1->position, p1->rotation, p1->scale).Inverse();
		m->contacts[0].normal = normal;
		m->contacts[0].penetration = penetration;
#if 0
		Render::DrawLine(p0->position+m->contacts[0].local0, p1->position+m->contacts[0].local1, Color_Yellow);
		Render::DrawLine(p0->position+m->contacts[0].local0, (p0->position+m->contacts[0].local0)+(normal*penetration), Color_Magenta);
		Log("physics","Collision between sphere and convex. mesh_point:",mesh_point," radius:",c0->radius," distance: ",distance);
#endif
	}
}

void ConvexMeshConvexMeshCollision(Physics* p0, Collider* c0, Physics* p1, Collider* c1, Manifold* m){
	
}

///////////////
//// @init ////
///////////////
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
	
	paused      = false;
	step        = false;
	solving     = true;
	integrating = true;
}

/////////////////
//// @update ////
/////////////////
void PhysicsSystem::Update(){
	if(paused && !step) return;
	
	fixedAccumulator += DeshTime->deltaTime;
	while(fixedAccumulator >= fixedDeltaTime){
		//// integration ////
		if(integrating){
			if(!AtmoAdmin->simulateInEditor) AtmoAdmin->player->Update();
			forE(AtmoAdmin->physicsArr){
				if(it->attribute.entity == AtmoAdmin->player) continue;
				
				//linear motion
				if(!it->staticPosition){
					it->acceleration += vec3(0, -gravity, 0); //add gravity
					it->velocity += it->acceleration * fixedDeltaTime;
					
					f32 vm = it->velocity.mag();
					if(vm > maxVelocity){
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
					if(it->rotVelocity != vec3::ZERO){ //fake rotational air friction
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
			if(p0->collider.type == ColliderType_NONE) continue;
			for(Physics* p1 = p0+1; p1 != AtmoAdmin->physicsArr.end(); ++p1){
				if(    (p1->collider.type != ColliderType_NONE) 
				   &&  (p0->collider.layer == p1->collider.layer) 
				   && !(p0->collider.playerOnly && p1->attribute.entity != AtmoAdmin->player) 
				   && !(p1->collider.playerOnly && p0->attribute.entity != AtmoAdmin->player)){
					manifolds.add(Manifold{p0->attribute.entity, p1->attribute.entity, p0, p1, &p0->collider, &p1->collider});
				}
			}
		}
		
		//// narrow collision detection //// (fill manifolds)
		forE(manifolds){
			switch(it->c0->type){
				case ColliderType_AABB:       switch(it->c1->type){
					case ColliderType_AABB:      { AABBAABBCollision      (it->p0, it->c0, it->p1, it->c1, it); }break;
					case ColliderType_Sphere:    { AABBSphereCollision    (it->p0, it->c0, it->p1, it->c1, it); }break;
					case ColliderType_ConvexMesh:{ AABBConvexMeshCollision(it->p0, it->c0, it->p1, it->c1, it); }break;
					default:{ Assert(!"not implemented"); }break;
				}break;
				case ColliderType_Sphere:     switch(it->c1->type){
					case ColliderType_AABB:      { AABBSphereCollision      (it->p1, it->c1, it->p0, it->c0, it); }break;
					case ColliderType_Sphere:    { SphereSphereCollision    (it->p0, it->c0, it->p1, it->c1, it); }break;
					case ColliderType_ConvexMesh:{ SphereConvexMeshCollision(it->p0, it->c0, it->p1, it->c1, it); }break;
					default:{ Assert(!"not implemented"); }break;
				}break;
				case ColliderType_ConvexMesh: switch(it->c1->type){
					case ColliderType_AABB:      { AABBConvexMeshCollision      (it->p1, it->c1, it->p0, it->c0, it); }break;
					case ColliderType_Sphere:    { SphereConvexMeshCollision    (it->p1, it->c1, it->p0, it->c0, it); }break;
					case ColliderType_ConvexMesh:{ ConvexMeshConvexMeshCollision(it->p0, it->c0, it->p1, it->c1, it); }break;
					default:{ Assert(!"not implemented"); }break;
				}break;
				default:{ Assert(!"not implemented"); }break;
			}
		}
		
		//// collision resolution //// (solve manifolds)
		if(solving){
			forE(manifolds){
				if(it->state == ContactState_NONE) continue;
				
				//set triggers as active
				if(it->c0->isTrigger) it->c0->triggerActive = true;
				if(it->c1->isTrigger) it->c1->triggerActive = true;
				
				forI(it->contactCount){
					vec3 normal = it->contacts[i].normal;
					f32  penetration = it->contacts[i].penetration;
					
					//static resolution
					if     (it->p0->staticPosition){ it->p1->position += normal*penetration; }
					else if(it->p1->staticPosition){ it->p0->position -= normal*penetration; }
					else{ it->p0->position -= normal*penetration/2.f; it->p1->position += normal*penetration/2.f; }
					
					//dynamic resolution; ref:https://www.euclideanspace.com/physics/dynamics/collision/threed/index.htm
					//TODO figure out why omega1 and thus scalar is wrong
					vec3 r0 = it->p0->position + it->contacts[i].local0;
					vec3 r1 = it->p1->position + it->contacts[i].local1;
					vec3 omega0 = normal.cross(r0) 
						* (it->c0->tensor.To4x4() * mat4::TransformationMatrix(it->p0->position, it->p0->rotation, it->p0->scale)).Inverse();
					vec3 omega1 = normal.cross(r1) 
						* (it->c1->tensor.To4x4() * mat4::TransformationMatrix(it->p1->position, it->p1->rotation, it->p1->scale)).Inverse();
					f32 scalar = (1.f/it->p0->mass) + (1.f/it->p1->mass) /*+ normal.dot(r0.cross(omega0) + r1.cross(omega1))*/;
					f32 rest_coef = (it->p0->elasticity + it->p1->elasticity) / 2.f; //NOTE not how restitution coef is calulated in real physics
					f32 impulse_mod = (1.f + rest_coef) * (it->p1->velocity - it->p0->velocity).mag() / scalar;
					vec3 impulse = normal * impulse_mod;
					
					if(!it->p0->staticPosition) it->p0->velocity -= impulse / it->p0->mass;
					if(!it->p1->staticPosition) it->p1->velocity += impulse / it->p1->mass;
					
					if(!(it->p0->staticRotation || it->c0->type == ColliderType_AABB)) it->p0->rotVelocity -= DEGREES(omega0);
					if(!(it->p1->staticRotation || it->c1->type == ColliderType_AABB)) it->p1->rotVelocity += DEGREES(omega1);
				}
			}
		}
		
		if(!AtmoAdmin->simulateInEditor) AtmoAdmin->player->PostCollisionUpdate();
		
		//// update fixed time ////
		fixedAccumulator -= fixedDeltaTime;
		fixedTotalTime += fixedDeltaTime;
		
		if(step){
			paused = true;
			step = false;
			fixedAccumulator = 0;
			break;
		}
	}
	fixedAlpha = fixedAccumulator / fixedDeltaTime;
}