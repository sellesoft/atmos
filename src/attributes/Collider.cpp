#include "Collider.h"
#include "math/InertiaTensors.h"
#include "core/model.h"

Collider::Collider(const Collider& rhs){
	memcpy(this, &rhs, sizeof(Collider));
}

void Collider::operator= (const Collider& rhs){
	memcpy(this, &rhs, sizeof(Collider));
}

void Collider::RecalculateTensor(f32 mass){
	Assert(mass > 0);
	switch(type){
		case ColliderType_AABB:{
			tensor = InertiaTensors::SolidCuboid(2.f*halfDims.x, 2.f*halfDims.y, 2.f*halfDims.z, mass); 
		}break;
		case ColliderType_Sphere:{
			tensor = InertiaTensors::SolidSphere(radius, mass); 
		}break;
	}
}


/////////
// 
//    AABB
// 
/////////
Collider AABBCollider(Mesh* mesh, f32 mass){
	Assert(mesh && mesh->vertexCount && mass > 0);
	Collider result{};
	result.type = ColliderType_AABB;
	
	vec3 min = mesh->aabbMin.absV();
	vec3 max = mesh->aabbMax.absV();
	result.halfDims.x = (min.x > max.x) ? min.x : max.x;
	result.halfDims.y = (min.y > max.y) ? min.y : max.y;
	result.halfDims.z = (min.z > max.z) ? min.z : max.z;
	result.tensor = InertiaTensors::SolidCuboid(2.f*result.halfDims.x, 2.f*result.halfDims.y, 2.f*result.halfDims.z, mass);
	return result;
}

Collider AABBCollider(vec3 _halfDims, f32 mass){
	Assert(_halfDims.x > 0 && _halfDims.y > 0 && _halfDims.z > 0 && mass > 0);
	Collider result{};
	result.type = ColliderType_AABB;
	result.halfDims = _halfDims;
	result.tensor = InertiaTensors::SolidCuboid(2.f*_halfDims.x, 2.f*_halfDims.y, 2.f*_halfDims.z, mass);
	return result;
}


/////////
// 
//    Sphere
// 
/////////
Collider SphereCollider(float _radius, f32 mass){
	Assert(_radius > 0 && mass > 0);
	Collider result{};
	result.type = ColliderType_Sphere;
	result.radius = _radius;
	result.tensor = InertiaTensors::SolidSphere(_radius,mass);
	return result;
}
