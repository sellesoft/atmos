#include "Collider.h"
#include "math/InertiaTensors.h"
#include "core/model.h"

/////////
// 
//    AABB
// 
/////////
AABBCollider::AABBCollider(Mesh* mesh, f32 mass){
	Assert(mesh && mesh->vertexCount && mass > 0);
	shape = ColliderShape_AABB;
	
	vec3 min = mesh->aabbMin.absV();
	vec3 max = mesh->aabbMax.absV();
	halfDims.x = (min.x > max.x) ? min.x : max.x;
	halfDims.y = (min.y > max.y) ? min.y : max.y;
	halfDims.z = (min.z > max.z) ? min.z : max.z;
	
	tensor = InertiaTensors::SolidCuboid(2.f*halfDims.x, 2.f*halfDims.y, 2.f*halfDims.z, mass);
}

AABBCollider::AABBCollider(vec3 _halfDims, f32 mass){
	Assert(_halfDims.x > 0 && _halfDims.y > 0 && _halfDims.z > 0 && mass > 0);
	shape    = ColliderShape_AABB;
	halfDims = _halfDims;
	tensor   = InertiaTensors::SolidCuboid(2.f*halfDims.x, 2.f*halfDims.y, 2.f*halfDims.z, mass);
}

void AABBCollider::RecalculateTensor(f32 mass){
	Assert(mass > 0);
	tensor = InertiaTensors::SolidCuboid(2.f*halfDims.x, 2.f*halfDims.y, 2.f*halfDims.z, mass);
}


/////////
// 
//    Sphere
// 
/////////
SphereCollider::SphereCollider(float _radius, f32 mass){
	Assert(_radius > 0 && mass > 0);
	shape  = ColliderShape_AABB;
	radius = _radius;
	tensor = InertiaTensors::SolidSphere(radius,mass);
}

void SphereCollider::RecalculateTensor(f32 mass){
	Assert(mass > 0);
	tensor = InertiaTensors::SolidSphere(radius,mass);
}
