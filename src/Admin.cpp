#include "Admin.h"
#include "entities/Entity.h"
#include "entities/PlayerEntity.h"
#include "entities/PhysicsEntity.h"
#include "core/storage.h"
#include "core/logging.h"
#include "core/window.h"

void Admin::Init(){
    state = GameState_Editor;
    simulateInEditor = false;
    
	camera = CameraInstance(90);
	controller.Init();
	editor.Init();
    physics.Init(300);
	
    player = new PlayerEntity;
    player->Init("player",0,Transform{vec3(10,20,10)},1.0f);
    
    {//sandbox
        Mesh* box_mesh = Storage::CreateBoxMesh(1,1,1,Color_Red).second;
        u32 flat_mat = Storage::CreateMaterial("flat", Shader_Flat, MaterialFlags_NONE, {}).first;
        Model* model = Storage::CreateModelFromMesh(box_mesh).second;
        model->batches[0].material = flat_mat;
        PhysicsEntity* box1 = new PhysicsEntity;
        box1->Init("floor",model,Transform{vec3(0,-2,0),vec3::ZERO,vec3(20,1,20)},1.0f,true);
        AtmoAdmin->entities.add(box1);
        
        PhysicsEntity* box2 = new PhysicsEntity;
        box2->Init("falling",Storage::NullModel(),Transform{vec3(0,10,0),vec3::ZERO,vec3::ONE},1.0f);
        AtmoAdmin->entities.add(box2);
    }
}

void Admin::Update(){
    switch(state){
        case GameState_Play:{
            controller.Update();
			player->Update();
            camera.Update();
            forE(movementArr) it->Update();
            physics.Update();
            f32 alpha = physics.fixedAccumulator / physics.fixedDeltaTime;
            forE(physicsArr)  it->Update(alpha);
            forE(modelArr)    it->Update();
        }break;
        
        case GameState_Menu:{
			controller.Update();
            forE(modelArr)    it->Update();
        }break;
        
        case GameState_Editor:{
            controller.Update();
            camera.Update();
            editor.Update();
            if(simulateInEditor){
                physics.Update();
                f32 alpha = physics.fixedAccumulator / physics.fixedDeltaTime;
                forE(physicsArr)  it->Update(alpha);
            }
            forE(modelArr)    it->Update();
        }break;
    }
}

void Admin::ChangeState(GameState new_state){
	if(state == new_state) return;
    if(state >= GameState_COUNT) return logE("Admin attempted to switch to unhandled gamestate: ", new_state);
	
	const char* from; const char* to;
	switch(state){
		case GameState_Play:    from = "PLAY";
		switch(new_state){
			case GameState_Menu:{   to = "MENU";
				DeshWindow->UpdateCursorMode(CursorMode_Default);
				player->model->visible = false;
				//TODO save binary
			}break;
			case GameState_Editor:{ to = "EDITOR";
				DeshWindow->UpdateCursorMode(CursorMode_Default);
				player->model->visible = true;
				//TODO save binary
				//TODO load text
			}break;
		}break;
		case GameState_Menu:    from = "MENU";
		switch(new_state){
			case GameState_Play:{   to = "PLAY";
				DeshWindow->UpdateCursorMode(CursorMode_FirstPerson);
				player->model->visible = false;
				//TODO save binary
			}break;
			case GameState_Editor:{ to = "EDITOR";
				DeshWindow->UpdateCursorMode(CursorMode_Default);
				player->model->visible = true;
				//TODO save binary
				//TODO load text
			}break;
		}break;
		case GameState_Editor:  from = "EDITOR";
		switch(new_state){
			case GameState_Play:{   to = "PLAY";
				DeshWindow->UpdateCursorMode(CursorMode_FirstPerson);
				player->model->visible = false;
				//TODO save text
			}break;
			case GameState_Menu:{   to = "MENU";
				DeshWindow->UpdateCursorMode(CursorMode_Default);
				player->model->visible = true;
				//TODO save text
			}break;
		}break;
	}
	
	state = new_state;
	logf("atmos","Changed gamestate from %s to %s", from,to);
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
					perp01 = normal.cross(p1 - p0).yInvert().normalized();
					perp12 = normal.cross(p2 - p1).yInvert().normalized();
					perp20 = normal.cross(p0 - p2).yInvert().normalized();
                    
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

