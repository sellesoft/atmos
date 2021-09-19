#include "Collider.h"
#include "math/InertiaTensors.h"
#include "core/console.h"
#include "core/model.h"
#include "core/logging.h"


/////////
// 
//    AABB
// 
/////////
AABBCollider::AABBCollider(Mesh* mesh, f32 mass, vec3 _offset, u32 collisionLayer, bool nocollide){
	shape     = ColliderShape_AABB;
	collLayer = collisionLayer;
	noCollide = nocollide;
	offset    = _offset;
    
	if (!mesh) { halfDims = vec3::ZERO; LogE("collider","null mesh passed for AABB creation"); return; }
	if (!mesh->vertexCount) { halfDims = vec3::ZERO; LogE("collider","mesh with no vertices passed to AABB creation"); return; }
    
	vec3 min = mesh->aabbMin.absV();
	vec3 max = mesh->aabbMax.absV();
	halfDims.x = (min.x > max.x) ? min.x : max.x;
	halfDims.y = (min.y > max.y) ? min.y : max.y;
	halfDims.z = (min.z > max.z) ? min.z : max.z;
    
	tensor    = InertiaTensors::SolidCuboid(2.f*abs(halfDims.x), 2.f*abs(halfDims.y), 2.f*abs(halfDims.z), mass);
}

AABBCollider::AABBCollider(vec3 _halfDims, f32 mass, vec3 _offset, u32 collisionLayer, bool nocollide){
	shape     = ColliderShape_AABB;
	collLayer = collisionLayer;
	noCollide = nocollide;
	halfDims  = _halfDims;
	offset    = _offset;
	tensor    = InertiaTensors::SolidCuboid(2.f*abs(halfDims.x), 2.f*abs(halfDims.y), 2.f*abs(halfDims.z), mass);
}

void AABBCollider::RecalculateTensor(f32 mass){
	tensor = InertiaTensors::SolidCuboid(2.f*abs(halfDims.x), 2.f*abs(halfDims.y), 2.f*abs(halfDims.z), mass);
}


/////////
// 
//    Sphere
// 
/////////
SphereCollider::SphereCollider(float _radius, f32 mass, vec3 _offset, u32 collisionLayer, bool nocollide){
	shape     = ColliderShape_AABB;
	collLayer = collisionLayer;
	noCollide = nocollide;
    radius    = _radius;
	offset    = _offset;
    tensor    = InertiaTensors::SolidSphere(radius,mass);
}

void SphereCollider::RecalculateTensor(f32 mass){
	tensor = InertiaTensors::SolidSphere(radius,mass);
}


/////////
// 
//    Complex
// 
/////////
ComplexCollider::ComplexCollider(Mesh* mesh, f32 mass, vec3 _offset, u32 collisionLayer, bool noCollide){
	//!!Incomplete
    
}
