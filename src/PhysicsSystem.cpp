#include "Admin.h"
#include "PhysicsSystem.h"
#include "core/storage.h"
#include "utils/array.h"
#include "entities/Entity.h"
#include "entities/PlayerEntity.h"
#include "attributes/Physics.h"
#include "attributes/Collider.h"
#include "geometry/geometry.h"

////////////////
//// @debug ////
////////////////
#include "core/renderer.h"
void DrawMeshFace(Mesh* mesh, MeshFace* face, mat4& transform, color c){
	vec3 prev = mesh->vertexes[face->outerVertexes[face->outerVertexCount-1]].pos * transform;
	for(u32 idx : face->outerVertexes){
		vec3 curr = mesh->vertexes[idx].pos * transform;
		Render::DrawLine(prev, curr, c);
		prev = curr;
	}
}

//////////////////
//// @utility ////
//////////////////
local const vec3 physics_aabb_normals[6] = { 
	vec3::RIGHT, vec3::UP,   vec3::FORWARD,
	vec3::LEFT,  vec3::DOWN, vec3::BACK,
};

//TODO encapsulation
FORCE_INLINE bool AABBAABBTest(vec3 min0, vec3 max0, vec3 min1, vec3 max1){
	return (min0.x <= max1.x && max0.x >= min1.x) && (min0.y <= max1.y && max0.y >= min1.y) && (min0.z <= max1.z && max0.z >= min1.z);
}

////////////// //!ref: https://www.gdcvault.com/play/1017646/Physics-for-Game-Programmers-The
//// @SAT //// //!ref: http://media.steampowered.com/apps/valve/2015/DirkGregorius_Contacts.pdf
////////////// //!ref: https://research.ncl.ac.uk/game/mastersdegree/gametechnologies/previousinformation/physics5collisionmanifolds/
struct FaceQuerySAT{ f32 penetration; u32 face; };
local FaceQuerySAT HullFaceQuerySAT(Mesh* m0, mat3& rotation0, mat4& transform0, Mesh* m1, mat3& rotation1, mat4& transform1){
	FaceQuerySAT result{-FLT_MAX};
	
	//transform local p0 into local p1 space (since we are getting vertex of p1)
	mat3 rotation = rotation0 * rotation1.Inverse();
	mat4 transform = transform0 * transform1.Inverse();
	
	forI(m0->faceCount){
		vec3 plane_point = m0->vertexes[m0->faces[i].vertexes[0]].pos * transform;
		vec3 plane_normal = m0->faces[i].normal * rotation;
		vec3 m1_vertex = FurthestHullVertexPositionAlongNormal(m1, -plane_normal);
		f32  penetration = Math::DistPointToPlane(m1_vertex, plane_normal, plane_point);
		if(penetration > result.penetration){
			result.penetration = penetration;
			result.face = i;
#if 0
			if(penetration > 0){
				Log("phys","No face collision between ",m0->name," and ",m1->name," on axis: ",m0->faces[i].normal*rotation0," penetration: ",penetration);
				vec3 scale{1000,1000,1000};
				Render::DebugLine((m0->faces[i].center*transform0)-((m0->faces[i].normal*rotation0)*scale),
								  (m0->faces[i].center*transform0)+((m0->faces[i].normal*rotation0)*scale), Color_Magenta);
			}
#endif
			if(penetration > 0) return result; //early out if no collision
		}
	}
	
	return result;
}

local constexpr f32 edge_parallel_tolerance = 0.005f;
struct EdgeQuerySAT{ f32 penetration; vec3 normal; u32 edge0_start; u32 edge0_end; u32 edge1_start; u32 edge1_end; };
local EdgeQuerySAT HullEdgeQuerySAT(Mesh* m0, mat3& rotation0, mat4& transform0, Mesh* m1, mat3& rotation1, mat4& transform1){
	EdgeQuerySAT result{-FLT_MAX};
	
	//transform local p0 into local p1 space
	mat3 rotation = rotation0 * rotation1.Inverse();
	mat4 transform = transform0 * transform1.Inverse();
	vec3 m0_center = {transform.data[12], transform.data[13], transform.data[14]};
	
	//NOTE cross all edge combinations, check if the axis makes minkowski difference face, check for penetration
	for(MeshFace& face0 : m0->faces){ //iterate mesh1 edges
		u32 idx0_prev = face0.outerVertexes[face0.outerVertexCount-1];
		vec3 vert0_prev = m0->vertexes[idx0_prev].pos * transform;
		for(u32 idx0_curr : face0.outerVertexes){
			vec3 vert0_curr = m0->vertexes[idx0_curr].pos * transform;
			vec3 edge0 = vert0_curr - vert0_prev;
			
			//get face0 normal and face0 edge-neighbor normal
			vec3 face0_normal = face0.normal * rotation;
			vec3 face0_neighbor_normal;
			for(u32 nei_idx : face0.faceNeighbors){
				b32 has_prev = false; b32 has_curr = false;
				for(u32 nei_vert_idx : m0->faces[nei_idx].outerVertexes){
					if(nei_vert_idx == idx0_prev) has_prev = true;
					if(nei_vert_idx == idx0_curr) has_curr = true;
				}
				if(has_prev && has_curr){
					face0_neighbor_normal = m0->faces[nei_idx].normal * rotation;
					break;
				}
			}
			
			for(MeshFace& face1 : m1->faces){ //iterate mesh1 edges
				u32 idx1_prev = face1.outerVertexes[face1.outerVertexCount-1];
				vec3 vert1_prev = m1->vertexes[idx1_prev].pos;
				for(u32 idx1_curr : face1.outerVertexes){
					vec3 vert1_curr = m1->vertexes[idx1_curr].pos;
					vec3 edge1 = vert1_curr - vert1_prev;
					
					//get face1 normal and face1 edge-neighbor normal
					vec3 face1_normal = face1.normal;
					vec3 face1_neighbor_normal;
					for(u32 nei_idx : face1.faceNeighbors){
						b32 has_prev = false; b32 has_curr = false;
						for(u32 nei_vert_idx : m1->faces[nei_idx].outerVertexes){
							if(nei_vert_idx == idx1_prev) has_prev = true;
							if(nei_vert_idx == idx1_curr) has_curr = true;
						}
						if(has_prev && has_curr){
							face1_neighbor_normal = m1->faces[nei_idx].normal;
							break;
						}
					}
					
					//skip axis if the edge pair doesnt make a face on the minkowski difference (test if arcs intersect on unit sphere)
					f32 cba = edge0.dot(-face1_normal); f32 dba = edge0.dot(-face1_neighbor_normal);
					f32 adc = edge1.dot( face0_normal); f32 bdc = edge1.dot( face0_neighbor_normal);
					if((cba*dba < 0.0f) && (adc*bdc < 0.0f) && (cba*bdc > 0.0f)){
						//make the edge vs edge axis
						vec3 axis = edge1.cross(edge0);
						f32 axis_length = axis.mag();
						
						//skip axis if the edges are basically parallel: |e1 x e2| = |e1| * |e2| * sin(alpha); alpha = 0
						f32 edge0_length = edge0.mag(); f32 edge1_length = edge1.mag();
						if(axis_length > edge_parallel_tolerance * sqrt(edge0_length*edge0_length * edge1_length*edge1_length)){
							//force normal to point p0 to p1
							vec3 normal = axis / axis_length;
							if(normal.dot(vert0_prev - m0_center) < 0) normal = -normal;
							
							//check if smallest penetration
							f32 penetration = normal.dot(vert1_prev - vert0_prev);
							if(penetration > result.penetration){
								result.penetration = penetration;
								result.normal      = normal;
								result.edge0_start = idx0_prev;
								result.edge0_end   = idx0_curr;
								result.edge1_start = idx1_prev;
								result.edge1_end   = idx1_curr;
								if(penetration > 0){
#if 0
									Log("phys","No edge collision between ",m0->name," and ",m1->name," on axis: ",normal*rotation1," penetration: ",penetration);
									vec3 scale{1000,1000,1000};
									Render::DebugLine(((m0_center/2)*transform1)-((normal*rotation1)*scale),
													  ((m0_center/2)*transform1)+((normal*rotation1)*scale), Color_Magenta);
									Render::DebugLine(m1->vertexes[idx1_prev].pos*transform1, m1->vertexes[idx1_curr].pos*transform1, Color_Yellow);
									Render::DebugLine(m0->vertexes[idx0_prev].pos*transform0, m0->vertexes[idx0_curr].pos*transform0, Color_Yellow);
#endif
									return result; //early out if no collision
								}
							}
						}
					}
					
					idx1_prev = idx1_curr;
					vert1_prev = vert1_curr;
				}
			}
			
			idx0_prev = idx0_curr;
			vert0_prev = vert0_curr;
		}
	}
	
	return result;
}

//!ref: https://en.wikipedia.org/wiki/Sutherland%E2%80%93Hodgman_algorithm
//!ref: https://gamedevelopment.tutsplus.com/tutorials/understanding-sutherland-hodgman-clipping-for-physics-engines--gamedev-11917
//TODO speedup by not using array
local void GenerateHullFaceCollisionManifoldSAT(Mesh* ref_mesh, mat3& ref_rotation, mat4& ref_transform, 
												Mesh* inc_mesh, mat3& inc_rotation, mat4& inc_transform, 
												FaceQuerySAT query, b32 p0_ref, Manifold* m){
	//NOTE clipping occurs in incident space
	mat4 transform = ref_transform * inc_transform.Inverse();
	mat4 rotation  = ref_rotation * inc_rotation.Inverse();
	MeshFace* ref_face = &ref_mesh->faces[query.face];
	MeshFace* inc_face = FurthestHullFaceAlongNormal(inc_mesh, -ref_face->normal * rotation);
	vec3 ref_plane_point  = ref_mesh->vertexes[ref_face->vertexes[0]].pos * transform;
	vec3 ref_plane_normal = ref_face->normal * rotation;
	
#if 0
	DrawMeshFace(ref_mesh, ref_face, ref_transform, Color_Red);
	DrawMeshFace(inc_mesh, inc_face, inc_transform, Color_Blue);
#endif
	
	//clip incident vertexes to reference face's neighbor planes
	array<vec3> clipped; for(u32 vert_idx : inc_face->outerVertexes){ clipped.add(inc_mesh->vertexes[vert_idx].pos); }
	for(u32 face_idx : ref_face->faceNeighbors){
		vec3 nei_plane_point  = ref_mesh->vertexes[ref_mesh->faces[face_idx].vertexes[0]].pos * transform;
		vec3 nei_plane_normal = ref_mesh->faces[face_idx].normal * rotation;
		
		array<vec3> temp = clipped; clipped.clear();
		vec3 prev = temp[temp.count-1];
		f32 dist_prev = Math::DistPointToPlane(prev, nei_plane_normal, nei_plane_point);
		for(vec3& curr : temp){
			f32 dist_curr = Math::DistPointToPlane(curr, nei_plane_normal, nei_plane_point);
			if      ((dist_prev < 0.0f) && (dist_curr < 0.0f)){
				clipped.add(prev);
			}else if((dist_prev > 0.0f) && (dist_curr < 0.0f)){
				clipped.add(Math::VectorPlaneIntersect(nei_plane_point, nei_plane_normal, prev, curr));
			}else if((dist_prev < 0.0f) && (dist_curr > 0.0f)){
				clipped.add(prev);
				clipped.add(Math::VectorPlaneIntersect(nei_plane_point, nei_plane_normal, prev, curr));
			}
			prev = curr;
			dist_prev = dist_curr;
		}
	}
	
#if 0
	forE(clipped) Render::DrawBox(mat4::TransformationMatrix(*it * inc_transform, vec3::ZERO, vec3{.05f,.05f,.05f}), Color_Green);
	return;
#endif
	
	//remove clipped vertexes behind reference face and place all clipped vertexes on that face
	array<f32>  clipped_dists;
	array<vec3> valid_clipped;
	forI(clipped.count){
		f32 dist = Math::DistPointToPlane(clipped[i], ref_plane_normal, ref_plane_point);
		if(dist < 0.0f){
			valid_clipped.add(clipped[i] - (ref_plane_normal * dist));
			clipped_dists.add(dist);
		}
	}
	clipped = valid_clipped;
	
#if 0
	forI(clipped.count){
		if(clipped_dists[i] > 0.0f) continue;
		Render::DrawBox(mat4::TransformationMatrix(clipped[i]*inc_transform, vec3::ZERO, vec3{.05f,.05f,.05f}), Color_Green);
		Render::DrawLine(clipped[i]*inc_transform, 
						 clipped[i]*inc_transform + (ref_plane_normal*inc_rotation)*clipped_dists[i], Color_Yellow);
	}
	return;
#endif
	
	//enforce four contact points while maintaining maximal area
	if(clipped.count > 4){
		m->contactCount = 4;
		
		//first point is the deepest
		u32 idx0 = -1; f32 idx0_dist = 0.0f;
		forI(clipped.count){ 
			if(clipped_dists[i] < idx0_dist){ 
				idx0 = i; 
				idx0_dist = clipped_dists[i]; 
			} 
		}
		vec3 point0 = clipped[idx0];
		
		//second point is furthest from first (using squared distance since its the same for comparison)
		u32 idx1 = -1; f32 idx1_dist = 0.0f;
		forI(clipped.count){ 
			if(idx0) continue;
			vec3 diff = clipped[i]-clipped[idx0];
			f32 dist = diff.dot(diff);
			if(dist > idx1_dist){ 
				idx1 = i; 
				idx1_dist = dist; 
			} 
		}
		vec3 point1 = clipped[idx1];
		
		//third point is the one that give the most positive area (using cross product to avoid sqrt)
		u32 idx2 = -1; f32 idx2_area = 0.0f;
		forI(clipped.count){ 
			if(idx0 || idx1) continue;
			f32 area = (ref_plane_normal.dot((clipped[i]-point1).cross(clipped[i]-point0))) * 0.5f;
			if(area > idx2_area){ 
				idx2 = i; 
				idx2_area = area; 
			} 
		}
		vec3 point2 = clipped[idx2];
		
		//TODO compare against all edges of triangle rather than first one
		//fourth point is the one that give the most negative area (since we already got most positive)
		u32 idx3 = -1; f32 idx3_area = 0.0f;
		forI(clipped.count){ 
			if(idx0 || idx1 || idx2) continue;
			f32 area = (ref_plane_normal.dot((clipped[i]-point1).cross(clipped[i]-point0))) * 0.5f;
			if(area < idx3_area){
				idx3 = i; 
				idx3_area = area;
			} 
		}
		vec3 point3 = clipped[idx3];
		
		m->contacts[0].position    = (point0 + (ref_plane_normal * clipped_dists[idx0])) * inc_transform;
		m->contacts[1].position    = (point1 + (ref_plane_normal * clipped_dists[idx1])) * inc_transform;
		m->contacts[2].position    = (point2 + (ref_plane_normal * clipped_dists[idx2])) * inc_transform;
		m->contacts[3].position    = (point3 + (ref_plane_normal * clipped_dists[idx3])) * inc_transform;
		m->contacts[0].penetration = clipped_dists[idx0];
		m->contacts[1].penetration = clipped_dists[idx1];
		m->contacts[2].penetration = clipped_dists[idx2];
		m->contacts[3].penetration = clipped_dists[idx3];
	}else{
		m->contactCount = 0;
		forI(clipped.count){
			m->contacts[m->contactCount].position    = (clipped[i] + (ref_plane_normal * clipped_dists[i])) * inc_transform;
			m->contacts[m->contactCount].penetration = clipped_dists[i];
			m->contactCount++;
		}
	}
	
	//ensure manifold isnt reversed
	if(p0_ref){
		m->normal = ref_face->normal * ref_rotation;
	}else{
		m->normal = -ref_face->normal * ref_rotation;
	}
	
#if 0
	forI(m->contactCount){
		Render::DrawLine(m->contacts[i].position, 
						 m->contacts[i].position + m->normal*m->contacts[i].penetration, Color_Yellow);
		//Render::DrawBox(mat4::TransformationMatrix(m->contacts[i].local0*inc_transform, vec3::ZERO, vec3{.05f,.05f,.05f}), Color_Red);
		//Render::DrawBox(mat4::TransformationMatrix(m->contacts[i].local1*ref_transform, vec3::ZERO, vec3{.05f,.05f,.05f}), Color_Blue);
		Render::DrawBox(mat4::TransformationMatrix(m->contacts[i].position, vec3::ZERO, vec3{.05f,.05f,.05f}), Color_Green);
	}
#endif
#if 1
	Log("phys","Hull-Hull face collision between ",m->p0->attribute.entity->name," and ",m->p1->attribute.entity->name,"."
		"\n  reference: ", m->p1->attribute.entity->name,
		"\n  normal:    ", m->normal,
		"\n  contacts:  ", m->contactCount);
	forI(m->contactCount){
		Log("","  contact",i,": position: ",m->contacts[i].position," penetration: ",m->contacts[i].penetration);
	}
#endif
}

//!ref: https://mathworld.wolfram.com/Line-LineIntersection.html
//!ref: https://stackoverflow.com/questions/34602761/intersecting-3d-lines
local void GenerateHullEdgeCollisionManifoldSAT(Mesh* mesh0, mat3& rotation0, mat4& transform0, 
												Mesh* mesh1, mat3& rotation1, mat4& transform1, 
												EdgeQuerySAT& query, Manifold* m){
	//NOTE edge query info is in p1 local space and penetration is negative
	//offset p0's edge to intersect with p1's edge in p1 space
	mat4 transform = transform0 * transform1.Inverse();
	vec3 offset = query.normal * query.penetration;
	vec3 edge0_start_offset = (mesh0->vertexes[query.edge0_start].pos * transform) + offset;
	vec3 edge0_end_offset   = (mesh0->vertexes[query.edge0_end  ].pos * transform) + offset;
	vec3 edge1_start = mesh1->vertexes[query.edge1_start].pos;
	vec3 edge1_end   = mesh1->vertexes[query.edge1_end  ].pos;
	
	//find intersect (we can assume non-parallel and intersecting due to setup)
	vec3 edge0 = edge0_end_offset - edge0_start_offset;
	vec3 edge1 = edge1_end - edge1_start;
	vec3 diff  = edge1_start - edge0_start_offset;
	f32  dot00 = edge0.dot(edge0);
	f32  dot01 = edge0.dot(edge1);
	f32  dot11 = edge1.dot(edge1);
	f32  dot0d = edge0.dot(diff);
	f32  dot1d = edge1.dot(diff);
	f32  discriminant = dot01 * dot01 - dot00 * dot11;
	f32  dist_along_edge0 = (dot01 * dot1d - dot11 * dot0d) / discriminant;
	f32  dist_along_edge1 = (dot00 * dot1d - dot01 * dot0d) / discriminant;
	vec3 edge0_point = edge0_start_offset + (edge0 * dist_along_edge0);
	vec3 edge1_point = edge1_start        + (edge1 * dist_along_edge1);
	Assert((edge1_point - edge0_point).mag() < .0001f, "the edges arent intersecting even though we offset them");
	
	m->contactCount = 1;
	m->normal = query.normal * rotation1;
	m->contacts[0].position = edge1_point.midpoint(edge0_point - offset) * transform1;
	m->contacts[0].penetration = query.penetration;
	
#if 0
	Render::DrawLine(mesh0->vertexes[query.edge0_start].pos * transform0, mesh0->vertexes[query.edge0_end  ].pos * transform0, Color_Red);
	Render::DrawLine(edge1_start * transform1, edge1_end   * transform1, Color_Blue);
	Render::DrawLine(m->contacts[0].position, m->contacts[0].position + m->normal*query.penetration, Color_Yellow);
	Render::DrawBox(mat4::TransformationMatrix((edge0_point - offset)*transform1, vec3::ZERO, vec3{.05f,.05f,.05f}), Color_Blue);
	Render::DrawBox(mat4::TransformationMatrix(edge1_point*transform1, vec3::ZERO, vec3{.05f,.05f,.05f}), Color_Red);
	Render::DrawBox(mat4::TransformationMatrix(m->contacts[0].position, vec3::ZERO, vec3{.05f,.05f,.05f}), Color_Green);
#endif
#if 1
	Log("phys","Hull-Hull edge collision between ",m->p0->attribute.entity->name," and ",m->p1->attribute.entity->name,"."
		"\n  normal:      ", m->normal,
		"\n  position:    ", m->contacts[0].position,
		"\n  penetration: ", m->contacts[0].penetration);
#endif
}

/////////////////////
//// @primitives ////
/////////////////////
//TODO encapsulation
void AABBAABBCollision(Physics* p0, Collider* c0, Physics* p1, Collider* c1, Manifold* m){
	vec3 position0 = p0->position + c0->offset;
	vec3 position1 = p1->position + c1->offset;
	vec3 min0 = position0 - (c0->halfDims * p0->scale);
	vec3 max0 = position0 + (c0->halfDims * p0->scale);
	vec3 min1 = position1 - (c1->halfDims * p1->scale);
	vec3 max1 = position1 + (c1->halfDims * p1->scale);
	
	if(AABBAABBTest(min0, max0, min1, max1)){
		//early out if no collide or both are static
		m->contactCount = -1;
		if(c0->noCollide || c1->noCollide) return;
		if(p0->staticPosition && p1->staticPosition && p0->staticRotation && p1->staticRotation) return;
		
		f32 distances[6] = { 
			min1.x - max0.x, min1.y - max0.y, min1.z - max0.z,
			min0.x - max1.x, min0.y - max1.y, min0.z - max1.z, 
		};
		f32 penetration = -FLT_MAX;
		vec3 normal;
		
		forI(6){
			if(distances[i] > penetration){
				penetration = distances[i]; 
				normal = physics_aabb_normals[i];
			}
		}
		m->contactCount = 1;
		m->normal = normal;
		m->contacts[0].position    = vec3::ZERO; //NOTE not needed for AABB-AABB resolution
		m->contacts[0].penetration = penetration;
#if 1
		Log("phys","AABB-AABB collision between ",p0->attribute.entity->name," and ",p1->attribute.entity->name,"."
			"\n  normal:      ", m->normal,
			"\n  position:    ", m->contacts[0].position,
			"\n  penetration: ", m->contacts[0].penetration);
#endif
	}
}

//TODO encapsulation
void AABBSphereCollision(Physics* p0, Collider* c0, Physics* p1, Collider* c1, Manifold* m){
	vec3 position0 = p0->position + c0->offset;
	vec3 position1 = p1->position + c1->offset;
	vec3 aabb_point = ClosestPointOnAABB(c0->halfDims * p0->scale, position1 - position0) + position0;
	vec3 delta =  position1 - aabb_point;
	f32  distance = delta.mag();
	
	if(distance < c1->radius){
		//early out if no collide or both are static
		m->contactCount = -1;
		if(c0->noCollide || c1->noCollide) return;
		if(p0->staticPosition && p1->staticPosition && p0->staticRotation && p1->staticRotation) return;
		
		f32 penetration = distance - c1->radius;
		vec3 normal = delta / distance;
		m->contactCount = 1;
		m->normal = normal;
		m->contacts[0].position    = aabb_point;
		m->contacts[0].penetration = penetration;
#if 1
		Log("phys","AABB-Sphere collision between ",p0->attribute.entity->name," and ",p1->attribute.entity->name,"."
			"\n  normal:      ", m->normal,
			"\n  position:    ", m->contacts[0].position,
			"\n  penetration: ", m->contacts[0].penetration);
#endif
	}
}

void AABBHullCollision(Physics* p0, Collider* c0, Physics* p1, Collider* c1, Manifold* m){
	vec3 position0 = p0->position + c0->offset;
	vec3 position1 = p1->position + c1->offset;
	mat3 rotation0  = mat3::RotationMatrix(vec3::ZERO);
	mat3 rotation1  = mat3::RotationMatrix(p1->rotation);
	mat4 transform0 = mat4::TransformationMatrix(position0, vec3::ZERO, p0->scale * (c0->halfDims / vec3{0.5f,0.5f,0.5f}));
	mat4 transform1 = mat4::TransformationMatrix(position1, p1->rotation, p1->scale);
	
	FaceQuerySAT face_query0 = HullFaceQuerySAT(Storage::NullMesh(), rotation0, transform0, c1->mesh, rotation1, transform1);
	if(face_query0.penetration >= 0) return; //no collision on mesh0 axes
	
	FaceQuerySAT face_query1 = HullFaceQuerySAT(c1->mesh, rotation1, transform1, Storage::NullMesh(), rotation0, transform0);
	if(face_query1.penetration >= 0) return; //no collision on mesh1 axes
	
	EdgeQuerySAT edge_query  = HullEdgeQuerySAT(Storage::NullMesh(), rotation0, transform0, c1->mesh, rotation1, transform1);
	if(edge_query.penetration  >= 0) return; //no collision on edge vs edge axes
	
	//early out if no collide or both are static
	m->contactCount = -1;
	if(c0->noCollide || c1->noCollide) return;
	if(p0->staticPosition && p1->staticPosition && p0->staticRotation && p1->staticRotation) return;
	
	if      (face_query0.penetration > face_query1.penetration && face_query0.penetration > edge_query.penetration){
		GenerateHullFaceCollisionManifoldSAT(Storage::NullMesh(), rotation0, transform0, 
											 c1->mesh, rotation1, transform1, face_query0, true,  m);
	}else if(face_query1.penetration > face_query0.penetration && face_query1.penetration > edge_query.penetration){
		GenerateHullFaceCollisionManifoldSAT(c1->mesh, rotation1, transform1, 
											 Storage::NullMesh(), rotation0, transform0, face_query1, false, m);
	}else{
		GenerateHullEdgeCollisionManifoldSAT(Storage::NullMesh(), rotation0, transform0, c1->mesh, rotation1, transform1, edge_query, m);
	}
}

//TODO encapsulation
void SphereSphereCollision(Physics* p0, Collider* c0, Physics* p1, Collider* c1, Manifold* m){
	f32  radii = c0->radius + c1->radius;
	vec3 delta = (p1->position + c1->offset) - (p0->position + c0->offset);
	f32  distance = delta.mag();
	
	if(distance < radii){
		//early out if no collide or both are static
		m->contactCount = -1;
		if(c0->noCollide || c1->noCollide) return;
		if(p0->staticPosition && p1->staticPosition && p0->staticRotation && p1->staticRotation) return;
		
		f32 penetration = distance - radii;
		vec3 normal = delta / distance;
		m->contactCount = 1;
		m->normal = normal;
		m->contacts[0].position    = (normal * c0->radius).midpoint(-normal * c1->radius);
		m->contacts[0].penetration = penetration;
#if 1
		Log("phys","Sphere-Sphere collision between ",p0->attribute.entity->name," and ",p1->attribute.entity->name,"."
			"\n  normal:      ", m->normal,
			"\n  position:    ", m->contacts[0].position,
			"\n  penetration: ", m->contacts[0].penetration);
#endif
	}
}

//TODO encapsulation
void SphereHullCollision(Physics* p0, Collider* c0, Physics* p1, Collider* c1, Manifold* m){
	vec3 position0 = p0->position + c0->offset;
	vec3 position1 = p1->position + c1->offset;
	mat4 mesh_transform = mat4::TransformationMatrix(position1, p1->rotation, p1->scale);
	vec3 mesh_space_mesh_point = ClosestPointOnHull(c1->mesh, position0 * mesh_transform.Inverse());
	vec3 mesh_point = mesh_space_mesh_point * mesh_transform;
	vec3 delta = mesh_point - position0;
	f32  distance = delta.mag();
	
#if 0
	Render::DrawLine(position0, position1, Color_Red);
	Render::DrawBox(mat4::TransformationMatrix(mesh_point, vec3::ZERO, vec3(.1f,.1f,.1f)), Color_Blue);
#endif
	
	if(distance < c0->radius){
		//early out if no collide or both are static
		m->contactCount = -1;
		if(c0->noCollide || c1->noCollide) return;
		if(p0->staticPosition && p1->staticPosition && p0->staticRotation && p1->staticRotation) return;
		
		f32 penetration = distance - c0->radius;
		vec3 normal = (mesh_point - position1).normalized();
		m->contactCount = 1;
		m->normal = normal;
		m->contacts[0].position    = mesh_point;
		m->contacts[0].penetration = penetration;
#if 0
		Render::DrawLine(m->contacts[0].position, m->contacts[0].position+(normal*penetration), Color_Yellow);
		Render::DrawBox(mat4::TransformationMatrix(m->contacts[0].position, vec3::ZERO, vec3{.05f,.05f,.05f}), Color_Green);
#endif
#if 1
		Log("phys","Sphere-Hull collision between ",p0->attribute.entity->name," and ",p1->attribute.entity->name,"."
			"\n  normal:      ", m->normal,
			"\n  position:    ", m->contacts[0].position,
			"\n  penetration: ", m->contacts[0].penetration);
#endif
	}
}

void HullHullCollision(Physics* p0, Collider* c0, Physics* p1, Collider* c1, Manifold* m){
	vec3 position0 = p0->position + c0->offset;
	vec3 position1 = p1->position + c1->offset;
	mat3 rotation0  = mat3::RotationMatrix(p0->rotation);
	mat3 rotation1  = mat3::RotationMatrix(p1->rotation);
	mat4 transform0 = mat4::TransformationMatrix(position0, p0->rotation, p0->scale);
	mat4 transform1 = mat4::TransformationMatrix(position1, p1->rotation, p1->scale);
	
	FaceQuerySAT face_query0 = HullFaceQuerySAT(c0->mesh, rotation0, transform0, c1->mesh, rotation1, transform1);
	if(face_query0.penetration >= 0) return; //no collision on mesh0 axes
	
	FaceQuerySAT face_query1 = HullFaceQuerySAT(c1->mesh, rotation1, transform1, c0->mesh, rotation0, transform0);
	if(face_query1.penetration >= 0) return; //no collision on mesh1 axes
	
	EdgeQuerySAT edge_query  = HullEdgeQuerySAT(c0->mesh, rotation0, transform0, c1->mesh, rotation1, transform1);
	if(edge_query.penetration  >= 0) return; //no collision on edge vs edge axes
	
	//early out if no collide or both are static
	m->contactCount = -1;
	if(c0->noCollide || c1->noCollide) return;
	if(p0->staticPosition && p1->staticPosition && p0->staticRotation && p1->staticRotation) return;
	
	if      (face_query0.penetration > face_query1.penetration && face_query0.penetration > edge_query.penetration){
		GenerateHullFaceCollisionManifoldSAT(c0->mesh, rotation0, transform0, c1->mesh, rotation1, transform1, face_query0, true,  m);
	}else if(face_query1.penetration > face_query0.penetration && face_query1.penetration > edge_query.penetration){
		GenerateHullFaceCollisionManifoldSAT(c1->mesh, rotation1, transform1, c0->mesh, rotation0, transform0, face_query1, false, m);
	}else{
		GenerateHullEdgeCollisionManifoldSAT(c0->mesh, rotation0, transform0, c1->mesh, rotation1, transform1, edge_query, m);
	}
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
	if(AtmoAdmin->simulateInEditor) fixedAccumulator = fixedDeltaTime;
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
					manifolds.add(Manifold{p0, p1, &p0->collider, &p1->collider});
				}
			}
		}
		
		//// narrow collision detection //// (fill manifolds)
		forE(manifolds){
			switch(it->c0->type){
				case ColliderType_AABB:   switch(it->c1->type){
					case ColliderType_AABB:  { AABBAABBCollision  (it->p0, it->c0, it->p1, it->c1, it); }break;
					case ColliderType_Sphere:{ AABBSphereCollision(it->p0, it->c0, it->p1, it->c1, it); }break;
					case ColliderType_Hull:  { AABBHullCollision  (it->p0, it->c0, it->p1, it->c1, it); }break;
					default:{ Assert(!"not implemented"); }break;
				}break;
				case ColliderType_Sphere: switch(it->c1->type){
					case ColliderType_AABB:  { AABBSphereCollision  (it->p1, it->c1, it->p0, it->c0, it); }break;
					case ColliderType_Sphere:{ SphereSphereCollision(it->p0, it->c0, it->p1, it->c1, it); }break;
					case ColliderType_Hull:  { SphereHullCollision  (it->p0, it->c0, it->p1, it->c1, it); }break;
					default:{ Assert(!"not implemented"); }break;
				}break;
				case ColliderType_Hull:   switch(it->c1->type){
					case ColliderType_AABB:  { AABBHullCollision  (it->p1, it->c1, it->p0, it->c0, it); }break;
					case ColliderType_Sphere:{ SphereHullCollision(it->p1, it->c1, it->p0, it->c0, it); }break;
					case ColliderType_Hull:  { HullHullCollision  (it->p0, it->c0, it->p1, it->c1, it); }break;
					default:{ Assert(!"not implemented"); }break;
				}break;
				default:{ Assert(!"not implemented"); }break;
			}
		}
		
		//// collision resolution //// (solve manifolds)
		if(solving){
			forE(manifolds){
				if(it->contactCount == -1) continue; //skip when specified (no collide or static)
				
				//set triggers as active
				if(it->c0->isTrigger) it->c0->triggerActive = true;
				if(it->c1->isTrigger) it->c1->triggerActive = true;
				
				vec3 normal = it->normal;
				mat4 transform0 = mat4::TransformationMatrix(it->p0->position + it->c0->offset, it->p0->rotation, it->p0->scale);
				mat4 transform1 = mat4::TransformationMatrix(it->p1->position + it->c1->offset, it->p1->rotation, it->p1->scale);
				
				forI(it->contactCount){
					
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
	if(AtmoAdmin->simulateInEditor) fixedAlpha = 1;
}