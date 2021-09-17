#include "Admin.h"
#include "entities/Entity.h"
#include "entities/PlayerEntity.h"

#include "core/storage.h"


void Admin::Init() {
	camera = CameraInstance(90);
	controller.Init();
	editor.Init();
	
	{//atmos sandbox
		player = new PlayerEntity();
		player->Init("player", Storage::NullMesh(), Transform{ vec3::ZERO, vec3::ZERO, vec3::ONE }, 10);

	}
}

void Admin::Update(){
	controller.Update();
	camera.Update();

	//TODO game states
	editor.Update();
	
	//update player's attributes 
	if (player) {
		player->movement.Update();
		player->model.Update();
	}

	//update all attribute arrays
	
	forE (movementArr) it->Update(); 
	forE (modelArr)    it->Update(); 
}

void Admin::PostRenderUpdate(){
	
}

void Admin::Reset(){
	
}

void Admin::Cleanup(){
	
}

Entity* Admin::EntityRaycast(vec3 origin, vec3 direction, f32 maxDistance, EntityType filter) {
	Entity* result = 0;
	f32 min_depth = INFINITY;
	f32 depth;
	vec3 p0, p1, p2, normal;
	vec3 intersect;
	vec3 perp01, perp12, perp20;
	mat4 transform, rotation;
	Mesh::Triangle* tri;
	for (Entity* e : entities) {
		if (!(filter & e->type)) {
			transform = e->transform.Matrix();
			rotation = mat4::RotationMatrix(e->transform.rotation);
			if (ModelInstance* mc = e->modelPtr) {
				if (!mc->visible) continue;
				forX(tri_idx, mc->mesh->triangleCount) {
					tri = &mc->mesh->triangleArray[tri_idx];
					p0 = tri->p[0] * transform;
					p1 = tri->p[1] * transform;
					p2 = tri->p[2] * transform;
					normal = tri->normal * rotation;

					//early out if triangle is not facing us
					if (normal.dot(p0 - origin) >= 0) continue;

					//find where on the plane defined by the triangle our raycast intersects
					depth = (p0 - origin).dot(normal) / direction.dot(normal);
					intersect = origin + (direction * depth);

					//early out if intersection is behind us
					if (depth <= 0) continue;

					//make vectors perpendicular to each edge of the triangle
					perp01 = normal.cross(p1 - p0).normalized();
					perp12 = normal.cross(p2 - p1).normalized();
					perp20 = normal.cross(p0 - p2).normalized();

					//check that the intersection point is within the triangle and its the closest triangle found so far
					if (perp01.dot(intersect - p0) > 0 &&
						perp12.dot(intersect - p1) > 0 &&
						perp20.dot(intersect - p2) > 0) {

						//if its the closest triangle so far we store its index
						if (depth < min_depth) {
							result = e;
							min_depth = depth;
							break;
						}
					}
				}
			}
		}
	}

	if (result && depth <= maxDistance) 
		return result;
	return 0;
}

