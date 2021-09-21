#include "Admin.h"
#include "entities/Entity.h"
#include "entities/PlayerEntity.h"
#include "entities/PhysicsEntity.h"
#include "entities/TriggerEntity.h"
#include "entities/DoorEntity.h"
#include "core/storage.h"
#include "core/logging.h"
#include "core/window.h"

void Admin::Init(string _dataPath){
	dataPath = _dataPath;
    state = GameState_Editor;
    simulateInEditor = false;
    
	camera = CameraInstance(90);
	controller.Init();
	editor.Init();
    physics.Init(300);
	
	//NOTE temp reserves before we arena them so their pointers dont change
	physicsArr.reserve(1024);
	modelArr.reserve(1024);
	interpTransformArr.reserve(1024);
	
    player = new PlayerEntity;
    player->Init("player",Transform(vec3(10,10,10)));
    
    {//sandbox
		Mesh* box_mesh = Storage::CreateBoxMesh(1,1,1,Color_Red).second;
		Model* flat_box = Storage::CreateModelFromMesh(box_mesh).second;
		flat_box->batches[0].material = Storage::CreateMaterial("flat", Shader_Flat, MaterialFlags_NONE, {}).first;
		Model* lava_box = Storage::CreateModelFromMesh(box_mesh).second;
		lava_box->batches[0].material = Storage::CreateMaterial("lava", Shader_Lavalamp, MaterialFlags_NONE, {}).first;
		
		//respawn trigger
		TriggerEntity* trigger0 = new TriggerEntity;
		trigger0->Init("respawn trigger", Transform(vec3{0,-20,0},vec3::ZERO,vec3{10000,5,10000}), new AABBCollider(box_mesh,1.0f));
		trigger0->events.add(Event_PlayerRespawn);
		trigger0->connections.add(player);
		
		//first island
		PhysicsEntity* floor0 = new PhysicsEntity;
		floor0->Init("floor0", Transform(vec3(0,-.5f,0),vec3::ZERO,vec3(50,1,50)), Storage::CopyModel(flat_box).second, new AABBCollider(box_mesh,1.0f),1.0f,true);
		DoorEntity* wall0 = new DoorEntity;
		wall0->Init("wall0", new AABBCollider(box_mesh,1.0f), Storage::CopyModel(flat_box).second, Transform(vec3(24,-5.5f,0),vec3(0,0,0),vec3(2,10,50)), Transform(vec3(24,8,0),vec3(0,0,0),vec3(2,20,50)), 2);
		DoorEntity* wall1 = new DoorEntity;
		wall1->Init("wall1", new AABBCollider(box_mesh,1.0f), Storage::CopyModel(flat_box).second, Transform(vec3(-24,-5.5f,0),vec3(0,0,0),vec3(2,10,50)), Transform(vec3(-24,8,0),vec3(0,0,0),vec3(2,20,50)), 2);
		DoorEntity* wall2 = new DoorEntity;
		wall2->Init("wall2", new AABBCollider(box_mesh,1.0f), Storage::CopyModel(flat_box).second, Transform(vec3(0,-5.5f,-24),vec3(0,0,0),vec3(50,10,2)), Transform(vec3(0,8,-24),vec3(0,0,0),vec3(50,20,2)), 2);
		
		//stairs
		PhysicsEntity* stairs0 = new PhysicsEntity;
		PhysicsEntity* stairs1 = new PhysicsEntity;
		PhysicsEntity* stairs2 = new PhysicsEntity;
		PhysicsEntity* stairs3 = new PhysicsEntity;
		PhysicsEntity* stairs4 = new PhysicsEntity;
		stairs0->Init("stairs0", Transform(vec3(20,3,30),vec3::ZERO,vec3(5,.5f,5)), Storage::CopyModel(lava_box).second, new AABBCollider(box_mesh,1.0f),1.0f,true);
		stairs1->Init("stairs1", Transform(vec3(10,6,30),vec3::ZERO,vec3(5,.5f,5)), Storage::CopyModel(lava_box).second, new AABBCollider(box_mesh,1.0f),1.0f,true);
		stairs2->Init("stairs2", Transform(vec3(0,9,30),vec3::ZERO,vec3(5,.5f,5)), Storage::CopyModel(lava_box).second, new AABBCollider(box_mesh,1.0f),1.0f,true);
		stairs3->Init("stairs3", Transform(vec3(-10,12,30),vec3::ZERO,vec3(5,.5f,5)), Storage::CopyModel(lava_box).second, new AABBCollider(box_mesh,1.0f),1.0f,true);
		stairs4->Init("stairs4", Transform(vec3(-20,15,30),vec3::ZERO,vec3(5,.5f,5)), Storage::CopyModel(lava_box).second, new AABBCollider(box_mesh,1.0f),1.0f,true);
		stairs0->model->visible = false;
		stairs1->model->visible = false;
		stairs2->model->visible = false;
		stairs3->model->visible = false;
		stairs4->model->visible = false;
		
		//island trigger
		TriggerEntity* trigger1 = new TriggerEntity;
		trigger1->Init("island trigger", Transform(vec3{0,.55f,0},vec3::ZERO,vec3{1,.5f,1}), new AABBCollider(box_mesh,1.0f), Storage::CopyModel(lava_box).second);
		trigger1->events.add(Event_ModelVisibleToggle);
		trigger1->events.add(Event_ToggleTriggerActive);
		trigger1->events.add(Event_ToggleDoor);
		trigger1->connections.add(stairs0);
		trigger1->connections.add(stairs1);
		trigger1->connections.add(stairs2);
		trigger1->connections.add(stairs3);
		trigger1->connections.add(stairs4);
		trigger1->connections.add(trigger1); //NOTE hide its own model
		trigger1->connections.add(wall0);
		trigger1->connections.add(wall1);
		trigger1->connections.add(wall2);
    }
}

void Admin::Update(){
	Assert(physicsArr.count <= 1024 && modelArr.count <= 1024 && interpTransformArr.count <= 1024,
		   "temp max limit before we arena them so their pointers dont change");
	
    switch(state){
        case GameState_Play:{
            controller.Update();
			forE(interpTransformArr) it->Update();
            physics.Update();
            camera.Update();
            forE(physicsArr) it->Update(physics.fixedAlpha);
			for(TriggerEntity* t : triggers) t->Update(); //TODO(delle) remove this with better trigger system
            forE(modelArr) it->Update();
        }break;
        
        case GameState_Menu:{
			controller.Update();
            forE(modelArr) it->Update();
        }break;
        
        case GameState_Editor:{
            controller.Update();
            editor.Update();
            if(simulateInEditor){
				forE(interpTransformArr) it->Update();
                physics.Update();
                forE(physicsArr) it->Update(physics.fixedAlpha);
				for(TriggerEntity* t : triggers) t->Update(); //TODO(delle) remove this with better trigger system
            }
			camera.Update();
            forE(modelArr) it->Update();
        }break;
    }
}

void Admin::PostRenderUpdate(){
    
}

void Admin::Reset(){
    
}

void Admin::Cleanup(){
    //TODO save game
}

void Admin::ChangeState(GameState new_state){
	if(state == new_state) return;
    if(state >= GameState_COUNT) return LogE("Admin attempted to switch to unhandled gamestate: ", new_state);
	
	const char* from = "ERROR"; const char* to = "ERROR";
	switch(state){
		case GameState_Play:    from = "PLAY";
		switch(new_state){
			case GameState_Menu:{   to = "MENU";
				DeshWindow->UpdateCursorMode(CursorMode_Default);
				player->model->visible = false;
				//TODO save game
			}break;
			case GameState_Editor:{ to = "EDITOR";
				DeshWindow->UpdateCursorMode(CursorMode_Default);
				player->model->visible = true;
				//TODO save game
				//TODO load level
			}break;
		}break;
		case GameState_Menu:    from = "MENU";
		switch(new_state){
			case GameState_Play:{   to = "PLAY";
				DeshWindow->UpdateCursorMode(CursorMode_FirstPerson);
				player->model->visible = false;
				//TODO save game
			}break;
			case GameState_Editor:{ to = "EDITOR";
				DeshWindow->UpdateCursorMode(CursorMode_Default);
				player->model->visible = true;
				//TODO save game
				//TODO load level
			}break;
		}break;
		case GameState_Editor:  from = "EDITOR";
		switch(new_state){
			case GameState_Play:{   to = "PLAY";
				DeshWindow->UpdateCursorMode(CursorMode_FirstPerson);
				player->model->visible = false;
				//TODO save level
			}break;
			case GameState_Menu:{   to = "MENU";
				DeshWindow->UpdateCursorMode(CursorMode_Default);
				player->model->visible = true;
				//TODO save level
			}break;
		}break;
	}
	
	state = new_state;
	Logf("atmos","Changed gamestate from %s to %s", from,to);
}

void Admin::SaveLevel(cstring level_name){
	string level_text; level_text.reserve(4092);
	
	//level headers
	level_text += TOSTRING(">level"
						   "\nname        \"",level_name,"\""
						   "\nlast_updated ",0); //TODO add last_updated once diff checking is setup
	
	level_text += TOSTRING("\n"
						   "\n>player"
						   "\n");
	
	level_text += "\n\n>entities";
	
	//write to file
	string level_path = dataPath + "levels/" + to_string(level_name) + ".level";
}

void Admin::LoadLevel(cstring level_name){
	Assert(!"not implemented yet");
}

Entity* Admin::EntityRaycast(vec3 origin, vec3 direction, f32 maxDistance, EntityType filter) {
	Assert(maxDistance > 0 && direction != vec3::ZERO);
	
	Entity* result = 0;
	f32 min_depth = maxDistance;
	f32 depth;
	vec3 p0, p1, p2, normal;
	vec3 intersect;
	vec3 perp01, perp12, perp20;
	mat4 transform, rotation;
	Mesh::Triangle* tri;
	for(Entity* e : entities){
		if(!(filter & e->type)){
			transform = e->transform.Matrix();
			rotation = mat4::RotationMatrix(e->transform.rotation);
			if(ModelInstance* mc = e->modelPtr){
				if(!mc->visible) continue;
				forX(tri_idx, mc->mesh->triangleCount){
					tri = &mc->mesh->triangleArray[tri_idx];
					p0 = tri->p[0] * transform;
					normal = tri->normal * rotation;
                    
					//early out if triangle is not facing us
					if(normal.dot(p0 - origin) >= 0) continue;
                    
					//find where on the plane defined by the triangle our raycast intersects
					depth = (p0 - origin).dot(normal) / direction.dot(normal);
					intersect = origin + (direction * depth);
                    
					//early out if intersection is behind us
					if(depth <= 0) continue;
					
					p1 = tri->p[1] * transform;
					p2 = tri->p[2] * transform;
                    
					//make vectors perpendicular to each edge of the triangle
					perp01 = normal.cross(p1 - p0).normalized();
					perp12 = normal.cross(p2 - p1).normalized();
					perp20 = normal.cross(p0 - p2).normalized();
                    
					//check that the intersection point is within the triangle and its the closest triangle found so far
					if(perp01.dot(intersect - p0) > 0 &&
					   perp12.dot(intersect - p1) > 0 &&
					   perp20.dot(intersect - p2) > 0){
						//Log("raycast",origin," ",intersect," ",depth);
                        
						//if its the closest triangle so far we store its index
						if(depth < min_depth){
							result = e;
							min_depth = depth;
							break;
						}
					}
				}
			}
		}
	}
    
	return (result) ? result : 0;
}

