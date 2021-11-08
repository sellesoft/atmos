#include "Admin.h"
#include "PhysicsSystem.h"
#include "core/storage.h"
#include "utils/array.h"
#include "entities/Entity.h"
#include "entities/PlayerEntity.h"
#include "attributes/Physics.h"
#include "attributes/Collider.h"
#include "geometry/geometry.h"

//////////////////
//// @storage ////
//////////////////
array<Manifold> manifolds;

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

//!ref: https://box2d.org/posts/2014/02/computing-a-basis/
//NOTE modified for left-handed cooridinates
local void ComputeNormalBasis(const vec3& a, vec3* b, vec3* c){
	if(abs(a.x) >= 0.57735f){
		b->set(a.z, 0.0f, -a.x);
	}else{
		b->set(0.0f, -a.z, a.y);
	}
	
	b->normalize();
	*c = b->cross(a);
}

//mix elasticity while allowing super bouncy objects
local f32 MixElasticity(Physics* p0, Physics* p1){
	if(p0->elasticity > p1->elasticity){
		return p0->elasticity;
	}else{
		return p1->elasticity;
	}
}

//mix friction while allowing for zero friction
local f32 MixFriction(Physics* p0, Physics* p1){
	return sqrt(p0->kineticFricCoef * p1->kineticFricCoef);
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
	mat4 transform    = ref_transform * inc_transform.Inverse();
	mat4 invTransform = transform.Inverse();
	mat4 rotation     = ref_rotation * inc_rotation.Inverse();
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
		
		if(clipped.count == 0) return;
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
		
		m->contacts[0].penetration = clipped_dists[idx0];
		m->contacts[1].penetration = clipped_dists[idx1];
		m->contacts[2].penetration = clipped_dists[idx2];
		m->contacts[3].penetration = clipped_dists[idx3];
		if(p0_ref){
			m->normal = ref_face->normal * ref_rotation;
			m->contacts[0].local0 = point0 * invTransform;
			m->contacts[1].local0 = point1 * invTransform;
			m->contacts[2].local0 = point2 * invTransform;
			m->contacts[3].local0 = point3 * invTransform;
			m->contacts[0].local1 = point0 + (ref_plane_normal * clipped_dists[idx0]);
			m->contacts[1].local1 = point1 + (ref_plane_normal * clipped_dists[idx1]);
			m->contacts[2].local1 = point2 + (ref_plane_normal * clipped_dists[idx2]);
			m->contacts[3].local1 = point3 + (ref_plane_normal * clipped_dists[idx3]);
			m->contacts[0].world  = m->contacts[0].local1 * inc_transform;
			m->contacts[1].world  = m->contacts[1].local1 * inc_transform;
			m->contacts[2].world  = m->contacts[2].local1 * inc_transform;
			m->contacts[3].world  = m->contacts[3].local1 * inc_transform;
		}else{
			m->normal = -ref_face->normal * ref_rotation;
			m->contacts[0].local0 = point0 + (ref_plane_normal * clipped_dists[idx0]);
			m->contacts[1].local0 = point1 + (ref_plane_normal * clipped_dists[idx1]);
			m->contacts[2].local0 = point2 + (ref_plane_normal * clipped_dists[idx2]);
			m->contacts[3].local0 = point3 + (ref_plane_normal * clipped_dists[idx3]);
			m->contacts[0].local1 = point0 * invTransform;
			m->contacts[1].local1 = point1 * invTransform;
			m->contacts[2].local1 = point2 * invTransform;
			m->contacts[3].local1 = point3 * invTransform;
			m->contacts[0].world  = m->contacts[0].local0 * inc_transform;
			m->contacts[1].world  = m->contacts[1].local0 * inc_transform;
			m->contacts[2].world  = m->contacts[2].local0 * inc_transform;
			m->contacts[3].world  = m->contacts[3].local0 * inc_transform;
		}
	}else{
		m->contactCount = clipped.count;
		if(p0_ref){
			m->normal = ref_face->normal * ref_rotation;
			forI(clipped.count){
				m->contacts[i].local0 = clipped[i] * invTransform;
				m->contacts[i].local1 = clipped[i] + (ref_plane_normal * clipped_dists[i]);
				m->contacts[i].world  = m->contacts[i].local1 * inc_transform;
				m->contacts[i].penetration = clipped_dists[i];
			}
		}else{
			m->normal = -ref_face->normal * ref_rotation;
			forI(clipped.count){
				m->contacts[i].local0 = clipped[i] + (ref_plane_normal * clipped_dists[i]);
				m->contacts[i].local1 = clipped[i] * invTransform;
				m->contacts[i].world  = m->contacts[i].local0 * inc_transform;
				m->contacts[i].penetration = clipped_dists[i];
			}
		}
	}
	//ComputeNormalBasis(m->normal, &m->tangent0, &m->tangent1);
	
#if 0
	forI(m->contactCount){
		Render::DrawLine(m->contacts[i].position, 
						 m->contacts[i].position + m->normal*m->contacts[i].penetration, Color_Yellow);
		Render::DrawBox(mat4::TransformationMatrix(m->contacts[i].local0*inc_transform, vec3::ZERO, vec3{.05f,.05f,.05f}), Color_Red);
		Render::DrawBox(mat4::TransformationMatrix(m->contacts[i].local1*ref_transform, vec3::ZERO, vec3{.05f,.05f,.05f}), Color_Blue);
		Render::DrawBox(mat4::TransformationMatrix(m->contacts[i].world, vec3::ZERO, vec3{.05f,.05f,.05f}), Color_Green);
	}
#endif
#if 1
	Log("phys","Hull-Hull face collision between ",m->p0->attribute.entity->name," and ",m->p1->attribute.entity->name,"."
		"\n  reference: ", m->p1->attribute.entity->name,
		"\n  normal:    ", m->normal,
		"\n  contacts:  ", m->contactCount);
	forI(m->contactCount){
		Log("","  contact",i,": position: ",m->contacts[i].world," penetration: ",m->contacts[i].penetration);
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
	mat4 invTransform = transform.Inverse();
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
	//ComputeNormalBasis(m->normal, &m->tangent0, &m->tangent1);
	m->contacts[0].local0 = (edge0_point - offset) * invTransform;
	m->contacts[0].local1 = edge1_point;
	m->contacts[0].world  = edge1_point.midpoint(edge0_point - offset) * transform1;
	m->contacts[0].penetration = query.penetration;
	
#if 0
	Render::DrawLine(mesh0->vertexes[query.edge0_start].pos * transform0, mesh0->vertexes[query.edge0_end  ].pos * transform0, Color_Red);
	Render::DrawLine(edge1_start * transform1, edge1_end   * transform1, Color_Blue);
	Render::DrawLine(m->contacts[0].world, m->contacts[0].position + m->normal*query.penetration, Color_Yellow);
	Render::DrawBox(mat4::TransformationMatrix((edge0_point - offset)*transform1, vec3::ZERO, vec3{.05f,.05f,.05f}), Color_Blue);
	Render::DrawBox(mat4::TransformationMatrix(edge1_point*transform1, vec3::ZERO, vec3{.05f,.05f,.05f}), Color_Red);
	Render::DrawBox(mat4::TransformationMatrix(m->contacts[0].position, vec3::ZERO, vec3{.05f,.05f,.05f}), Color_Green);
#endif
#if 1
	Log("phys","Hull-Hull edge collision between ",m->p0->attribute.entity->name," and ",m->p1->attribute.entity->name,"."
		"\n  normal:      ", m->normal,
		"\n  position:    ", m->contacts[0].world,
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
		
		//NOTE contact points not needed for AABB-AABB resolution
		m->contactCount = 1;
		m->normal = normal;
		//ComputeNormalBasis(m->normal, &m->tangent0, &m->tangent1);
		m->contacts[0].penetration = penetration; 
#if 1
		Log("phys","AABB-AABB collision between ",p0->attribute.entity->name," and ",p1->attribute.entity->name,"."
			"\n  normal:      ", m->normal,
			"\n  position:    ", m->contacts[0].world,
			"\n  penetration: ", m->contacts[0].penetration);
#endif
	}
}

//TODO encapsulation
void AABBSphereCollision(Physics* p0, Collider* c0, Physics* p1, Collider* c1, Manifold* m){
	vec3 position0 = p0->position + c0->offset;
	vec3 position1 = p1->position + c1->offset;
	vec3 aabbPointAabbSpace = ClosestPointOnAABB(c0->halfDims * p0->scale, position1 - position0);
	vec3 aabbPoint = aabbPointAabbSpace + position0;
	vec3 delta =  position1 - aabbPoint;
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
		//ComputeNormalBasis(m->normal, &m->tangent0, &m->tangent1);
		m->contacts[0].local0 = aabbPointAabbSpace;
		m->contacts[0].local1 = -normal * c1->radius;
		m->contacts[0].world  = aabbPoint;
		m->contacts[0].penetration = penetration;
#if 1
		Log("phys","AABB-Sphere collision between ",p0->attribute.entity->name," and ",p1->attribute.entity->name,"."
			"\n  normal:      ", m->normal,
			"\n  position:    ", m->contacts[0].world,
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
	
	FaceQuerySAT faceQuery0 = HullFaceQuerySAT(Storage::NullMesh(), rotation0, transform0, c1->mesh, rotation1, transform1);
	if(faceQuery0.penetration >= 0) return; //no collision on mesh0 axes
	
	FaceQuerySAT faceQuery1 = HullFaceQuerySAT(c1->mesh, rotation1, transform1, Storage::NullMesh(), rotation0, transform0);
	if(faceQuery1.penetration >= 0) return; //no collision on mesh1 axes
	
	EdgeQuerySAT edgeQuery  = HullEdgeQuerySAT(Storage::NullMesh(), rotation0, transform0, c1->mesh, rotation1, transform1);
	if(edgeQuery.penetration  >= 0) return; //no collision on edge vs edge axes
	
	//early out if no collide or both are static
	m->contactCount = -1;
	if(c0->noCollide || c1->noCollide) return;
	if(p0->staticPosition && p1->staticPosition && p0->staticRotation && p1->staticRotation) return;
	
	if      (faceQuery0.penetration > faceQuery1.penetration && faceQuery0.penetration > edgeQuery.penetration){
		GenerateHullFaceCollisionManifoldSAT(Storage::NullMesh(), rotation0, transform0, 
											 c1->mesh, rotation1, transform1, faceQuery0, true,  m);
	}else if(faceQuery1.penetration > faceQuery0.penetration && faceQuery1.penetration > edgeQuery.penetration){
		GenerateHullFaceCollisionManifoldSAT(c1->mesh, rotation1, transform1, 
											 Storage::NullMesh(), rotation0, transform0, faceQuery1, false, m);
	}else{
		GenerateHullEdgeCollisionManifoldSAT(Storage::NullMesh(), rotation0, transform0, c1->mesh, rotation1, transform1, edgeQuery, m);
	}
}

//TODO encapsulation
void SphereSphereCollision(Physics* p0, Collider* c0, Physics* p1, Collider* c1, Manifold* m){
	vec3 position0 = p0->position + c0->offset;
	vec3 position1 = p1->position + c1->offset;
	f32  radii = c0->radius + c1->radius;
	vec3 delta = position1 - position0;
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
		//ComputeNormalBasis(m->normal, &m->tangent0, &m->tangent1);
		m->contacts[0].local0 = normal * c0->radius;
		m->contacts[0].local1 = -normal * c1->radius;
		m->contacts[0].world  = (m->contacts[0].local0 + position0).midpoint(m->contacts[0].local1 + position1);
		m->contacts[0].penetration = penetration;
#if 1
		Log("phys","Sphere-Sphere collision between ",p0->attribute.entity->name," and ",p1->attribute.entity->name,"."
			"\n  normal:      ", m->normal,
			"\n  position:    ", m->contacts[0].world,
			"\n  penetration: ", m->contacts[0].penetration);
#endif
	}
}

//TODO encapsulation
void SphereHullCollision(Physics* p0, Collider* c0, Physics* p1, Collider* c1, Manifold* m){
	vec3 position0 = p0->position + c0->offset;
	vec3 position1 = p1->position + c1->offset;
	mat4 meshTransform = mat4::TransformationMatrix(position1, p1->rotation, p1->scale);
	vec3 meshSpaceMeshPoint = ClosestPointOnHull(c1->mesh, position0 * meshTransform.Inverse());
	vec3 meshPoint = meshSpaceMeshPoint * meshTransform;
	vec3 delta = meshPoint - position0;
	f32  distance = delta.mag();
	
#if 0
	Render::DrawLine(position0, position1, Color_Red);
	Render::DrawBox(mat4::TransformationMatrix(meshPoint, vec3::ZERO, vec3(.1f,.1f,.1f)), Color_Blue);
#endif
	
	if(distance < c0->radius){
		//early out if no collide or both are static
		m->contactCount = -1;
		if(c0->noCollide || c1->noCollide) return;
		if(p0->staticPosition && p1->staticPosition && p0->staticRotation && p1->staticRotation) return;
		
		f32 penetration = distance - c0->radius;
		vec3 normal = (meshPoint - position0).normalized();
		m->contactCount = 1;
		m->normal = normal;
		//ComputeNormalBasis(m->normal, &m->tangent0, &m->tangent1);
		m->contacts[0].local0 = normal * c0->radius;
		m->contacts[0].local1 = meshSpaceMeshPoint;
		m->contacts[0].world  = meshPoint;
		m->contacts[0].penetration = penetration;
#if 0
		Render::DrawLine(m->contacts[0].position, m->contacts[0].position+(normal*penetration), Color_Yellow);
		Render::DrawBox(mat4::TransformationMatrix(m->contacts[0].position, vec3::ZERO, vec3{.05f,.05f,.05f}), Color_Green);
#endif
#if 1
		Log("phys","Sphere-Hull collision between ",p0->attribute.entity->name," and ",p1->attribute.entity->name,"."
			"\n  normal:      ", m->normal,
			"\n  position:    ", m->contacts[0].world,
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
	
	FaceQuerySAT faceQuery0 = HullFaceQuerySAT(c0->mesh, rotation0, transform0, c1->mesh, rotation1, transform1);
	if(faceQuery0.penetration >= 0) return; //no collision on mesh0 axes
	
	FaceQuerySAT faceQuery1 = HullFaceQuerySAT(c1->mesh, rotation1, transform1, c0->mesh, rotation0, transform0);
	if(faceQuery1.penetration >= 0) return; //no collision on mesh1 axes
	
	EdgeQuerySAT edgeQuery  = HullEdgeQuerySAT(c0->mesh, rotation0, transform0, c1->mesh, rotation1, transform1);
	if(edgeQuery.penetration  >= 0) return; //no collision on edge vs edge axes
	
	//early out if no collide or both are static
	m->contactCount = -1;
	if(c0->noCollide || c1->noCollide) return;
	if(p0->staticPosition && p1->staticPosition && p0->staticRotation && p1->staticRotation) return;
	
	if      (faceQuery0.penetration > faceQuery1.penetration && faceQuery0.penetration > edgeQuery.penetration){
		GenerateHullFaceCollisionManifoldSAT(c0->mesh, rotation0, transform0, c1->mesh, rotation1, transform1, faceQuery0, true,  m);
	}else if(faceQuery1.penetration > faceQuery0.penetration && faceQuery1.penetration > edgeQuery.penetration){
		GenerateHullFaceCollisionManifoldSAT(c1->mesh, rotation1, transform1, c0->mesh, rotation0, transform0, faceQuery1, false, m);
	}else{
		GenerateHullEdgeCollisionManifoldSAT(c0->mesh, rotation0, transform0, c1->mesh, rotation1, transform1, edgeQuery, m);
	}
}

///////////////
//// @init ////
///////////////
void PhysicsSystem::Init(f32 fixedUpdatesPerSecond){
	gravity        = 9.81f;
	
	fixedTimeStep    = fixedUpdatesPerSecond;
	fixedDeltaTime   = 1.f / fixedUpdatesPerSecond;
	fixedTotalTime   = 0;
	fixedUpdateCount = 0;
	fixedAccumulator = 0;
	
	paused      = false;
	step        = false;
	solving     = true;
	integrating = true;
	
	minVelocity    = 0.002f;
	maxVelocity    = 100.f;
	minRotVelocity = 1.f;
	maxRotVelocity = 360.f;
	
	velocityIterations  = 8;
	positionIterations  = 3;
	baumgarte           = 0.2f;
	linearSlop          = 0.005f;
	angularSlop         = (2.0f / 180.0f * M_PI); //2 degrees in radians
	restitutionSlop     = 0.05f;
	maxLinearCorrection = 0.2f;
}

/////////////////!ref: http://allenchou.net/2013/12/game-physics-resolution-contact-constraints/
//// @update ////!ref: https://github.com/erincatto/box2d/blob/master/src/dynamics/b2_contact_solver.cpp
/////////////////
void PhysicsSystem::Update(){
	if(paused && !step) return;
	
	fixedAccumulator += DeshTime->deltaTime;
	if(AtmoAdmin->simulateInEditor) fixedAccumulator = fixedDeltaTime;
	while(fixedAccumulator >= fixedDeltaTime){
		
		//// broad collision detection //// (filter manifolds)
		manifolds.clear();
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
					case ColliderType_AABB:  { AABBAABBCollision(it->p0, it->c0, it->p1, it->c1, it); }break;
					case ColliderType_Sphere:{ AABBSphereCollision(it->p0, it->c0, it->p1, it->c1, it); }break;
					case ColliderType_Hull:  { AABBHullCollision(it->p0, it->c0, it->p1, it->c1, it); }break;
					default:{ Assert(!"not implemented"); }break;
				}break;
				case ColliderType_Sphere: switch(it->c1->type){
					case ColliderType_AABB:  { Swap(it->p0, it->p1); Swap(it->c0, it->c1); AABBSphereCollision(it->p0, it->c0, it->p1, it->c1, it); }break;
					case ColliderType_Sphere:{ SphereSphereCollision(it->p0, it->c0, it->p1, it->c1, it); }break;
					case ColliderType_Hull:  { SphereHullCollision(it->p0, it->c0, it->p1, it->c1, it); }break;
					default:{ Assert(!"not implemented"); }break;
				}break;
				case ColliderType_Hull:   switch(it->c1->type){
					case ColliderType_AABB:  { Swap(it->p0, it->p1); Swap(it->c0, it->c1); AABBHullCollision(it->p0, it->c0, it->p1, it->c1, it); }break;
					case ColliderType_Sphere:{ Swap(it->p0, it->p1); Swap(it->c0, it->c1); SphereHullCollision(it->p0, it->c0, it->p1, it->c1, it); }break;
					case ColliderType_Hull:  { HullHullCollision(it->p0, it->c0, it->p1, it->c1, it); }break;
					default:{ Assert(!"not implemented"); }break;
				}break;
				default:{ Assert(!"not implemented"); }break;
			}
		}
		
		if(!AtmoAdmin->simulateInEditor) AtmoAdmin->player->Update();
		
		//// integrate linear and angular velocity ////
		if(integrating){
			forE(AtmoAdmin->physicsArr){
				if(it->attribute.entity == AtmoAdmin->player) continue;
				
				//linear motion
				if(!it->staticPosition){
					it->acceleration += vec3(0, -gravity, 0); //add gravity
					it->velocity += it->acceleration * fixedDeltaTime;
					it->acceleration = vec3::ZERO;
				}
				
				//rotational motion
				if(!it->staticRotation){
					if(it->rotVelocity != vec3::ZERO){  //fake rotational air friction
						it->rotAcceleration += vec3(it->rotVelocity.x > 0 ? -1 : 1, 
													it->rotVelocity.y > 0 ? -1 : 1, 
													it->rotVelocity.z > 0 ? -1 : 1) * it->airFricCoef * it->mass;
					}
					
					it->rotVelocity += it->rotAcceleration * fixedDeltaTime;
					it->rotAcceleration = vec3::ZERO;
				}
			}
		}
		
		//// initialize contact velocity contraints ////
		//!ref: https://github.com/bulletphysics/bullet3/blob/master/src/Bullet3Dynamics/ConstraintSolver/b3PgsJacobiSolver.cpp solveGroupCacheFriendlySetup()
		forE(manifolds){
			if(it->contactCount == 0 || it->contactCount == -1) continue; //skip when specified (no collide or static)
			
			//set triggers as active
			if(it->c0->isTrigger) it->c0->triggerActive = true;
			if(it->c1->isTrigger) it->c1->triggerActive = true;
			
			mat4 transform0 = mat4::TransformationMatrix(it->p0->position + it->c0->offset, it->p0->rotation, it->p0->scale);
			mat4 transform1 = mat4::TransformationMatrix(it->p1->position + it->c1->offset, it->p1->rotation, it->p1->scale);
			it->invMass0 = 1.f / it->p0->mass;
			it->invMass1 = 1.f / it->p1->mass;
			it->invInertia0 = it->c0->tensor.Inverse() * transform0;
			it->invInertia1 = it->c1->tensor.Inverse() * transform1;
			it->friction = MixFriction(it->p0, it->p1);
			
			forI(it->contactCount){
				{//// contact normal ////
					//compute angular components
					it->contacts[i].normalCrossLocal0 = it->normal.cross(it->contacts[i].local0);
					it->contacts[i].normalCrossLocal1 = it->normal.cross(it->contacts[i].local1);
					
					//compute effective mass for normal
					f32 denom0 = it->normal.dot((it->normal * it->invMass0) + (it->contacts[i].normalCrossLocal0 * it->invInertia0));
					f32 denom1 = it->normal.dot((it->normal * it->invMass1) + (it->contacts[i].normalCrossLocal1 * it->invInertia1));
					it->contacts[i].normalMass = 1.0f / (denom0 + denom1);
					
					//compute velocity bias from restitution //TODO maybe factor in positional error here?
					f32 relVel = it->normal.dot(it->p1->velocity) - it->contacts[i].normalCrossLocal1.dot(RADIANS(it->p1->rotVelocity))
						- it->normal.dot(it->p0->velocity) - it->contacts[i].normalCrossLocal0.dot(RADIANS(it->p0->rotVelocity));
					it->contacts[i].velocityBias = Max(0.0f, (-relVel * MixElasticity(it->p0, it->p1)) - restitutionSlop) / it->contactCount;
				}
				
				{//// contact friction ////
					vec3 vel0 = it->p0->velocity + it->contacts[i].local0.cross(RADIANS(it->p0->rotVelocity));
					vec3 vel1 = it->p1->velocity + it->contacts[i].local1.cross(RADIANS(it->p1->rotVelocity));
					vec3 vel = vel0 - vel1;
					f32  relVel = vel.dot(it->normal);
					
					it->contacts[i].tangent0 = vel - (it->normal * relVel);
					f32 lat_rel_vel = it->contacts[i].tangent0.magSq();
					if(lat_rel_vel > M_EPSILON){
						//compute tangent directions
						it->contacts[i].tangent0 /= sqrtf(lat_rel_vel);
						it->contacts[i].tangent1 = it->normal.cross(it->contacts[i].tangent0).normalized();
						
						//compute angular components
						it->contacts[i].tangent0CrossLocal0 = it->contacts[i].tangent0.cross(it->contacts[i].local0);
						it->contacts[i].tangent0CrossLocal1 = it->contacts[i].tangent0.cross(it->contacts[i].local1);
						it->contacts[i].tangent1CrossLocal0 = it->contacts[i].tangent1.cross(it->contacts[i].local0);
						it->contacts[i].tangent1CrossLocal1 = it->contacts[i].tangent1.cross(it->contacts[i].local1);
						
						//compute effective mass for tangents  //TODO maybe scale frictional effective mass by number of manifolds its in
						f32 denom0t0 = it->contacts[i].tangent0.dot((it->contacts[i].tangent0 * it->invMass0) 
																	+ (it->contacts[i].tangent0CrossLocal0 * it->invInertia0));
						f32 denom1t0 = it->contacts[i].tangent0.dot((it->contacts[i].tangent0 * it->invMass1) 
																	+ (it->contacts[i].tangent0CrossLocal1 * it->invInertia1));
						f32 denom0t1 = it->contacts[i].tangent1.dot((it->contacts[i].tangent1 * it->invMass0) 
																	+ (it->contacts[i].tangent1CrossLocal0 * it->invInertia0));
						f32 denom1t1 = it->contacts[i].tangent1.dot((it->contacts[i].tangent1 * it->invMass1) 
																	+ (it->contacts[i].tangent1CrossLocal1 * it->invInertia1));
						it->contacts[i].tangentMass0 = 1.0f / (denom0t0 + denom1t0);
						it->contacts[i].tangentMass1 = 1.0f / (denom0t1 + denom1t1);
					}else{
						it->contacts[i].tangent0 = vec3::ZERO;
					}
				}
			}
		}
		
		//// solve contact velocity constraints ////
		if(solving){
			forX(vel_it,velocityIterations){
				forE(manifolds){
					if(it->contactCount == 0 || it->contactCount == -1) continue; //skip when specified (no collide or static)
					
					//solve normal contact constraints
					if(true || it->contactCount == 1){
						forI(it->contactCount){
							//relative velocity at contact
							f32 relVel = it->normal.dot(it->p1->velocity) - it->contacts[i].normalCrossLocal1.dot(RADIANS(it->p1->rotVelocity))
								- it->normal.dot(it->p0->velocity) - it->contacts[i].normalCrossLocal0.dot(RADIANS(it->p0->rotVelocity));
							
							//compute normal force
							f32 lambda = (it->contacts[i].velocityBias - relVel) * it->contacts[i].normalMass;
							
							//clamp the accumulated impulse  //NOTE this allows solving the constraint across multiple iterations
							f32 newImpulse = Max(0.0f, it->contacts[i].normalImpulse + lambda);
							lambda = newImpulse - it->contacts[i].normalImpulse;
							it->contacts[i].normalImpulse = newImpulse;
							
							//apply contact impulse
							vec3 impulse = it->normal * lambda;
							if(!it->p0->staticPosition) it->p0->velocity -= impulse * it->invMass0;
							if(!it->p1->staticPosition) it->p1->velocity += impulse * it->invMass1;
							if(!it->p0->staticRotation) it->p0->rotVelocity -= it->contacts[i].local0.cross(impulse) * it->invInertia0;
							if(!it->p1->staticRotation) it->p1->rotVelocity += it->contacts[i].local1.cross(impulse) * it->invInertia1;
							/*
							if(it->contacts[i].tangent0 == vec3::ZERO || it->contacts[i].normalImpulse < M_EPSILON) continue;
							
							//relative velocity at contact
							f32 relVel0 = it->contacts[i].tangent0.dot(it->p1->velocity) - it->contacts[i].tangent0.dot(it->p0->velocity)
								- it->contacts[i].tangent0CrossLocal1.dot(RADIANS(it->p1->rotVelocity)) 
								- it->contacts[i].tangent0CrossLocal0.dot(RADIANS(it->p0->rotVelocity));
							f32 relVel1 = it->contacts[i].tangent1.dot(it->p1->velocity) - it->contacts[i].tangent1.dot(it->p0->velocity)
								- it->contacts[i].tangent1CrossLocal1.dot(RADIANS(it->p1->rotVelocity))
								- it->contacts[i].tangent1CrossLocal0.dot(RADIANS(it->p0->rotVelocity));
							
							//compute tangent force  //TODO factor tangent speed with conveyer belts
							f32 lambda0 = -relVel0 * it->contacts[i].tangentMass0;
							f32 lambda1 = -relVel1 * it->contacts[i].tangentMass1;
							
							//clamp the accumulated impulse  //NOTE this allows solving the constraint across multiple iterations
							f32 maxFriction = it->friction * it->contacts[i].normalImpulse;
							f32 newImpulse0 = Clamp(it->contacts[i].tangentImpulse0 + lambda0, -maxFriction, maxFriction);
							f32 newImpulse1 = Clamp(it->contacts[i].tangentImpulse1 + lambda1, -maxFriction, maxFriction);
							lambda0 = newImpulse0 - it->contacts[i].tangentImpulse0;
							lambda1 = newImpulse1 - it->contacts[i].tangentImpulse1;
							it->contacts[i].tangentImpulse0 = newImpulse0;
							it->contacts[i].tangentImpulse1 = newImpulse1;
							
							//apply contact impulse
							vec3 impulse0 = it->contacts[i].tangent0 * lambda0;
							vec3 impulse1 = it->contacts[i].tangent1 * lambda1;
							if(!it->p0->staticPosition) it->p0->velocity -= impulse0 * it->invMass0;
							if(!it->p0->staticPosition) it->p0->velocity -= impulse1 * it->invMass0;
							if(!it->p1->staticPosition) it->p1->velocity += impulse0 * it->invMass1;
							if(!it->p1->staticPosition) it->p1->velocity += impulse1 * it->invMass1;
							if(!it->p0->staticRotation) it->p0->rotVelocity -= it->contacts[i].local0.cross(impulse0) * it->invInertia0;
							if(!it->p0->staticRotation) it->p0->rotVelocity -= it->contacts[i].local0.cross(impulse1) * it->invInertia0;
							if(!it->p1->staticRotation) it->p1->rotVelocity += it->contacts[i].local1.cross(impulse0) * it->invInertia1;
							if(!it->p1->staticRotation) it->p1->rotVelocity += it->contacts[i].local1.cross(impulse1) * it->invInertia1;
							*/
						}
					}else{
						//TODO block solve with multiple contact points
					}
					
					//solve tangent contact contraints (friction) 
					forI(it->contactCount){
						if(it->contacts[i].tangent0 == vec3::ZERO || it->contacts[i].normalImpulse < M_EPSILON) continue;
						
						//relative velocity at contact
						f32 relVel0 = it->contacts[i].tangent0.dot(it->p1->velocity) - it->contacts[i].tangent0.dot(it->p0->velocity)
							- it->contacts[i].tangent0CrossLocal1.dot(RADIANS(it->p1->rotVelocity)) 
							- it->contacts[i].tangent0CrossLocal0.dot(RADIANS(it->p0->rotVelocity));
						f32 relVel1 = it->contacts[i].tangent1.dot(it->p1->velocity) - it->contacts[i].tangent1.dot(it->p0->velocity)
							- it->contacts[i].tangent1CrossLocal1.dot(RADIANS(it->p1->rotVelocity))
							- it->contacts[i].tangent1CrossLocal0.dot(RADIANS(it->p0->rotVelocity));
						
						//compute tangent force  //TODO factor tangent speed with conveyer belts
						f32 lambda0 = -relVel0 * it->contacts[i].tangentMass0;
						f32 lambda1 = -relVel1 * it->contacts[i].tangentMass1;
						
						//clamp the accumulated impulse  //NOTE this allows solving the constraint across multiple iterations
						f32 maxFriction = it->friction * it->contacts[i].normalImpulse;
						f32 newImpulse0 = Clamp(it->contacts[i].tangentImpulse0 + lambda0, -maxFriction, maxFriction);
						f32 newImpulse1 = Clamp(it->contacts[i].tangentImpulse1 + lambda1, -maxFriction, maxFriction);
						lambda0 = newImpulse0 - it->contacts[i].tangentImpulse0;
						lambda1 = newImpulse1 - it->contacts[i].tangentImpulse1;
						it->contacts[i].tangentImpulse0 = newImpulse0;
						it->contacts[i].tangentImpulse1 = newImpulse1;
						
						//apply contact impulse
						vec3 impulse0 = it->contacts[i].tangent0 * lambda0;
						vec3 impulse1 = it->contacts[i].tangent1 * lambda1;
						it->p0->velocity -= impulse0 * it->invMass0;
						it->p0->velocity -= impulse1 * it->invMass0;
						it->p1->velocity += impulse0 * it->invMass1;
						it->p1->velocity += impulse1 * it->invMass1;
						it->p0->rotVelocity -= it->contacts[i].local0.cross(impulse0) * it->invInertia0;
						it->p0->rotVelocity -= it->contacts[i].local0.cross(impulse1) * it->invInertia0;
						it->p1->rotVelocity += it->contacts[i].local1.cross(impulse0) * it->invInertia1;
						it->p1->rotVelocity += it->contacts[i].local1.cross(impulse1) * it->invInertia1;
					}
				}
			}
		}
		
		//// integrate position and rotation ////
		if(integrating){
			forE(AtmoAdmin->physicsArr){
				if(it->attribute.entity == AtmoAdmin->player) continue;
				
				//clamp linear velocity
				if(abs(it->velocity.x) < minVelocity) it->velocity.x = 0.0f;
				if(abs(it->velocity.y) < minVelocity) it->velocity.y = 0.0f;
				if(abs(it->velocity.z) < minVelocity) it->velocity.z = 0.0f;
				if(it->velocity.x > maxVelocity) it->velocity.x = maxVelocity;
				if(it->velocity.y > maxVelocity) it->velocity.y = maxVelocity;
				if(it->velocity.z > maxVelocity) it->velocity.z = maxVelocity;
				if(it->velocity.x < -maxVelocity) it->velocity.x = -maxVelocity;
				if(it->velocity.y < -maxVelocity) it->velocity.y = -maxVelocity;
				if(it->velocity.z < -maxVelocity) it->velocity.z = -maxVelocity;
				
				//clamp angular velocity
				if(abs(it->rotVelocity.x) < minRotVelocity) it->rotVelocity.x = 0.0f;
				if(abs(it->rotVelocity.y) < minRotVelocity) it->rotVelocity.y = 0.0f;
				if(abs(it->rotVelocity.z) < minRotVelocity) it->rotVelocity.z = 0.0f;
				if(it->rotVelocity.x > maxRotVelocity) it->rotVelocity.x = it->rotVelocity.x - maxRotVelocity*2.f;
				if(it->rotVelocity.y > maxRotVelocity) it->rotVelocity.y = it->rotVelocity.y - maxRotVelocity*2.f;
				if(it->rotVelocity.z > maxRotVelocity) it->rotVelocity.z = it->rotVelocity.z - maxRotVelocity*2.f;
				if(it->rotVelocity.x < -maxRotVelocity) it->rotVelocity.x = it->rotVelocity.x + maxRotVelocity*2.f;
				if(it->rotVelocity.y < -maxRotVelocity) it->rotVelocity.y = it->rotVelocity.y + maxRotVelocity*2.f;
				if(it->rotVelocity.z < -maxRotVelocity) it->rotVelocity.z = it->rotVelocity.z + maxRotVelocity*2.f;
				
				//integrate
				if(!it->staticPosition) it->position += it->velocity * fixedDeltaTime;
				if(!it->staticRotation) it->rotation += it->rotVelocity * fixedDeltaTime;
			}
		}
		
		//// solve contact position constraints //// //TODO figure out space
		if(solving){
			forX(pos_it,positionIterations){
				f32 minSeparation = 0.0f;
				forE(manifolds){
					if(it->contactCount == 0 || it->contactCount == -1) continue; //skip when specified (no collide or static)
					
					forI(it->contactCount){
						//track min separation to loop until resolution
						minSeparation = Min(minSeparation, it->contacts[i].penetration);
						
						//prevent large corrections and allow slop
						f32 numer = Clamp(baumgarte * (it->contacts[i].penetration + linearSlop), -maxLinearCorrection, 0.0f);
						
						//compute effective mass and impulse magnitude
						vec3 r0n = it->normal.cross(it->contacts[i].local0);
						vec3 r1n = it->normal.cross(it->contacts[i].local1);
						f32 denom = it->invMass0 + it->invMass1 + it->normal.dot(  it->contacts[i].local0.cross(r0n * it->invInertia0) 
																				 + it->contacts[i].local1.cross(r1n * it->invInertia1));
						f32 lambda = (denom > 0.0f) ? -numer / denom : 0.0f;
						
						//apply normal impulse
						vec3 impulse = it->normal * lambda;
						if(!it->p0->staticPosition) it->p0->position -= impulse * it->invMass0;
						if(!it->p1->staticPosition) it->p1->position += impulse * it->invMass1;
						if(!it->p0->staticRotation) it->p0->rotation -= impulse.cross(it->contacts[i].local0) * it->invInertia0;
						if(!it->p1->staticRotation) it->p1->rotation += impulse.cross(it->contacts[i].local1) * it->invInertia1;
					}
				}
				if(minSeparation >= -3.0f * linearSlop) break; //early out if solved
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