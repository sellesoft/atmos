#include "Collider.h"
#include "math/InertiaTensors.h"
#include "core/console.h"
#include "core/model.h"



/////////
// 
//    AABB
// 
/////////



AABBCollider::AABBCollider(Mesh* mesh, mat3 tensor_, u32 collisionLayer, bool nocollide) {
	shape     = ColliderShape_AABB;
	collLayer = collisionLayer;
	noCollide = nocollide;

	if (!mesh)              { halfDims = vec3::ZERO; ERROR("null mesh passed for AABB creation"); return; }
	if (!mesh->vertexCount) { halfDims = vec3::ZERO; ERROR("mesh with no vertices passed to AABB creation"); return; }

	vec3 min = mesh->aabbMin.absV();
	vec3 max = mesh->aabbMax.absV();

	halfDims.x = (min.x > max.x) ? min.x : max.x;
	halfDims.y = (min.y > max.y) ? min.y : max.y;
	halfDims.z = (min.z > max.z) ? min.z : max.z;

	tensor = tensor_;

}

AABBCollider::AABBCollider(vec3 halfDims_, mat3 tensor_, u32 collisionLayer, bool nocollide){
	shape     = ColliderShape_AABB;
	collLayer = collisionLayer;
	noCollide = nocollide;
	halfDims  = halfDims_;
	tensor    = tensor_;
}

AABBCollider::AABBCollider(Mesh* mesh, f32 mass, u32 collisionLayer, bool nocollide){
	shape     = ColliderShape_AABB;
	collLayer = collisionLayer;
	noCollide = nocollide;

	if (!mesh) { halfDims = vec3::ZERO; ERROR("null mesh passed for AABB creation"); return; }
	if (!mesh->vertexCount) { halfDims = vec3::ZERO; ERROR("mesh with no vertices passed to AABB creation"); return; }

	vec3 min = mesh->aabbMin.absV();
	vec3 max = mesh->aabbMax.absV();

	halfDims.x = (min.x > max.x) ? min.x : max.x;
	halfDims.y = (min.y > max.y) ? min.y : max.y;
	halfDims.z = (min.z > max.z) ? min.z : max.z;

	tensor    = InertiaTensors::SolidCuboid(2 * abs(halfDims.x), 2 * abs(halfDims.y), 2 * abs(halfDims.z), mass);


}

AABBCollider::AABBCollider(vec3 halfDims_, f32 mass, u32 collisionLayer, bool nocollide){
	shape     = ColliderShape_AABB;
	collLayer = collisionLayer;
	noCollide = nocollide;
	halfDims  = halfDims_;
	tensor    = InertiaTensors::SolidCuboid(2 * abs(halfDims.x), 2 * abs(halfDims.y), 2 * abs(halfDims.z), mass);

}

void AABBCollider::RecalculateTensor(f32 mass){
	tensor = InertiaTensors::SolidCuboid(2 * abs(halfDims.x), 2 * abs(halfDims.y), 2 * abs(halfDims.z), mass);
}



/////////
// 
//    Sphere
// 
/////////



SphereCollider::SphereCollider(float radius, mat3& tensor, u32 collisionLayer, bool noCollide){
	//!!Incomplete

}

SphereCollider::SphereCollider(float radius, f32 mass, mat3& tensor, u32 collisionLayer, bool noCollide){
	//!!Incomplete


}

void SphereCollider::RecalculateTensor(f32 mass){
	//!!Incomplete

}



/////////
// 
//    Complex
// 
/////////



ComplexCollider::ComplexCollider(Mesh* mesh, u32 collisionLayer, bool noCollide){
	//!!Incomplete

}
