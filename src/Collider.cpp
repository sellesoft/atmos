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
		case ColliderType_Hull:{
			tensor = InertiaTensors::SolidCuboid(mesh->aabbMax.x - mesh->aabbMin.x, mesh->aabbMax.y - mesh->aabbMin.y, 
												 mesh->aabbMax.z - mesh->aabbMin.z, mass);
		}break;
	}
}

Collider AABBCollider(Mesh* mesh, f32 mass){
	Assert(mesh && mesh->vertexCount && mass > 0);
	Collider result{};
	result.type = ColliderType_AABB;
	result.halfDims.x = (mesh->aabbMax.x - mesh->aabbMin.x)/2.f;
	result.halfDims.y = (mesh->aabbMax.y - mesh->aabbMin.y)/2.f;
	result.halfDims.z = (mesh->aabbMax.z - mesh->aabbMin.z)/2.f;
	result.RecalculateTensor(mass);
	return result;
}

Collider AABBCollider(vec3 halfDims, f32 mass){
	Assert(halfDims.x > 0 && halfDims.y > 0 && halfDims.z > 0 && mass > 0);
	Collider result{};
	result.type = ColliderType_AABB;
	result.halfDims = halfDims;
	result.RecalculateTensor(mass);
	return result;
}

Collider SphereCollider(f32 radius, f32 mass){
	Assert(radius > 0 && mass > 0);
	Collider result{};
	result.type = ColliderType_Sphere;
	result.radius = radius;
	result.RecalculateTensor(mass);
	return result;
}

//NOTE treating the inertia tensor as a cube for now
//TODO generic mesh inertia tensor generation
Collider HullCollider(Mesh* mesh, f32 mass){
	Assert(mesh && mass > 0);
	Collider result{};
	result.type = ColliderType_Hull;
	result.mesh = mesh;
	result.RecalculateTensor(mass);
	return result;
}