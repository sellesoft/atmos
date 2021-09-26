#include "Admin.h"
#include "entities/Entity.h"
#include "entities/PlayerEntity.h"
#include "entities/PhysicsEntity.h"
#include "entities/TriggerEntity.h"
#include "entities/SceneryEntity.h"
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
	
	/*
    {//sandbox
		player = new PlayerEntity;
		player->Init(Transform(vec3(10,10,10)));
		
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
		floor0->Init("floor0", Transform(vec3(0,-.5f,0),vec3::ZERO,vec3(50,1,50)), flat_box, new AABBCollider(box_mesh,1.0f),1.0f,true);
		DoorEntity* wall0 = new DoorEntity;
		wall0->Init("wall0", new AABBCollider(box_mesh,1.0f), flat_box, Transform(vec3(24,-5.5f,0),vec3(0,0,0),vec3(2,10,50)), Transform(vec3(24,8,0),vec3(0,0,0),vec3(2,20,50)), 2);
		DoorEntity* wall1 = new DoorEntity;
		wall1->Init("wall1", new AABBCollider(box_mesh,1.0f), flat_box, Transform(vec3(-24,-5.5f,0),vec3(0,0,0),vec3(2,10,50)), Transform(vec3(-24,8,0),vec3(0,0,0),vec3(2,20,50)), 2);
		DoorEntity* wall2 = new DoorEntity;
		wall2->Init("wall2", new AABBCollider(box_mesh,1.0f), flat_box, Transform(vec3(0,-5.5f,-24),vec3(0,0,0),vec3(50,10,2)), Transform(vec3(0,8,-24),vec3(0,0,0),vec3(50,20,2)), 2);
		
		//stairs
		PhysicsEntity* stairs0 = new PhysicsEntity;
		PhysicsEntity* stairs1 = new PhysicsEntity;
		PhysicsEntity* stairs2 = new PhysicsEntity;
		PhysicsEntity* stairs3 = new PhysicsEntity;
		PhysicsEntity* stairs4 = new PhysicsEntity;
		stairs0->Init("stairs0", Transform(vec3(20,3,30),vec3::ZERO,vec3(5,.5f,5)), lava_box, new AABBCollider(box_mesh,1.0f),1.0f,true);
		stairs1->Init("stairs1", Transform(vec3(10,6,30),vec3::ZERO,vec3(5,.5f,5)), lava_box, new AABBCollider(box_mesh,1.0f),1.0f,true);
		stairs2->Init("stairs2", Transform(vec3(0,9,30),vec3::ZERO,vec3(5,.5f,5)), lava_box, new AABBCollider(box_mesh,1.0f),1.0f,true);
		stairs3->Init("stairs3", Transform(vec3(-10,12,30),vec3::ZERO,vec3(5,.5f,5)), lava_box, new AABBCollider(box_mesh,1.0f),1.0f,true);
		stairs4->Init("stairs4", Transform(vec3(-20,15,30),vec3::ZERO,vec3(5,.5f,5)), lava_box, new AABBCollider(box_mesh,1.0f),1.0f,true);
		stairs0->model->visible = false;
		stairs1->model->visible = false;
		stairs2->model->visible = false;
		stairs3->model->visible = false;
		stairs4->model->visible = false;
		
		//island trigger
		TriggerEntity* trigger1 = new TriggerEntity;
		trigger1->Init("island trigger", Transform(vec3{0,.55f,0},vec3::ZERO,vec3{1,.5f,1}), new AABBCollider(box_mesh,1.0f), lava_box);
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
	*/
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
	camera = CameraInstance(90);
	editor.Reset();
	
	for(Entity* e : entities){ delete e; }
	entities.clear();
	triggers.clear();
	
    physicsArr.clear();
    modelArr.clear();
    interpTransformArr.clear();
	
	//NOTE temp reserves before we arena them so their pointers dont change
	physicsArr.reserve(1024);
	modelArr.reserve(1024);
	interpTransformArr.reserve(1024);
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
	string level; level.reserve(16380);
	
	//level
	//TODO add last_updated once diff checking is setup
	//TODO maybe add attribute counts to reserve array sizes to
	level += TOSTRING(">level"
					  "\nname         \"",level_name,"\""
					  "\nlast_updated ",0);
	
	//entities
	level += "\n\n>entities";
	forX(ent_idx,entities.count){
		//generic entity parts
		level += TOSTRING("\n]",entities[ent_idx]->id,
						  "\ntype     ",entities[ent_idx]->type," #",EntityTypeStrings[entities[ent_idx]->type],
						  "\nname     \"",entities[ent_idx]->name,"\""
						  "\nposition ",entities[ent_idx]->transform.position,
						  "\nrotation ",entities[ent_idx]->transform.rotation,
						  "\nscale    ",entities[ent_idx]->transform.scale);
		level += "\nattributes";
		if(entities[ent_idx]->player)   level += TOSTRING(" ",AttributeType_Player);
		if(entities[ent_idx]->movement) level += TOSTRING(" ",AttributeType_Movement);
		if(entities[ent_idx]->model)    level += TOSTRING(" ",AttributeType_ModelInstance);
		if(entities[ent_idx]->physics)  level += TOSTRING(" ",AttributeType_Physics);
		if(entities[ent_idx]->interp)   level += TOSTRING(" ",AttributeType_InterpTransform);
		if(entities[ent_idx]->connections.count){
			level += "\nconnections";
			for(Entity* e : entities[ent_idx]->connections){
				level += " ";
				level += to_string(e->id);
			}
		}
		
		//specific entity parts
		switch(entities[ent_idx]->type){
			case EntityType_Player:{
				PlayerEntity* e = (PlayerEntity*)entities[ent_idx];
				ModelInstance::SaveText(e->model, level);
				Physics::SaveText(e->physics, level);
			}break;
			case EntityType_Physics:{
				PhysicsEntity* e = (PhysicsEntity*)entities[ent_idx];
				ModelInstance::SaveText(e->model, level);
				Physics::SaveText(e->physics, level);
			}break;
			case EntityType_Scenery:{
				SceneryEntity* e = (SceneryEntity*)entities[ent_idx];
				ModelInstance::SaveText(e->model, level);
			}break;
			case EntityType_Trigger:{
				TriggerEntity* e = (TriggerEntity*)entities[ent_idx];
				if(e->events.count){
					level += "\nevents";
					for(u32 event : e->events){ level += " "; level += to_string(event); }
				}
				if(e->model) ModelInstance::SaveText(e->model, level);
				Physics::SaveText(e->physics, level);
			}break;
			case EntityType_Door:{
				DoorEntity* e = (DoorEntity*)entities[ent_idx];
				ModelInstance::SaveText(e->model, level);
				Physics::SaveText(e->physics, level);
				InterpTransform::SaveText(e->interp, level);
			}break;
		}
		level += "\n";
	}
	
	//write to file
	string level_path = dataPath + "levels/" + to_string(level_name) + ".level";
	Assets::writeFileBinary(level_path.str, level.str, level.count);
}

#define ParseError(...) LogE("admin-level","Error parsing level '",level_name,"' on line '",line_number,"'!",__VA_ARGS__)
void Admin::LoadLevel(cstring level_name){
	//load file
	string level_path = dataPath + "levels/" + to_string(level_name) + ".level";
	char* buffer = Assets::readFileAsciiToArray(level_path.str);
	if(!buffer) return;
	defer{ delete[] buffer; };
	
	//reset current level
	Reset();
	
	//parsing vars
	enum{PARSE_INVALID, PARSE_LEVEL, PARSE_ENTITIES, PARSE_ENTITY, PARSE_ATTRIBUTE};
	u32 parse_section = PARSE_INVALID;
	Type attribute_type;
	u32 entity_id = -1;
	Entity* entity = 0;
	array<pair<u32,array<u32>>> entity_connections;
	u32 interp_stage = -1;
	
	//load level
	char* line_start;  char* line_end = buffer-1;
	char* info_start;  char* info_end;
	char* key_start;   char* key_end;
	char* value_start; char* value_end;
	bool has_cr = false;
	for(u32 line_number = 1; ;line_number++){
		//get the next line
		line_start = (has_cr) ? line_end+2 : line_end+1;
		if((line_end = strchr(line_start, '\n')) == 0) break; //EOF if no '\n'
		if(has_cr || *(line_end-1) == '\r'){ has_cr = true; line_end -= 1; }
		if(line_start == line_end) continue;
		
		//format the line
		info_start = line_start + Utils::skipSpacesLeading(line_start, line_end-line_start);  if(info_start == line_end) continue;
		info_end   = info_start + Utils::skipComments(info_start, "#", line_end-info_start);  if(info_start == info_end) continue;
		info_end   = info_start + Utils::skipSpacesTrailing(info_start, info_end-info_start); if(info_start == info_end) continue;
		cstring info{info_start, u64(info_end-info_start)};
		
		//check for headers
		if      (*info_start == '>'){ //section
			if     (info == cstr_lit(">level"))   { parse_section = PARSE_LEVEL; }
			else if(info == cstr_lit(">entities")){ parse_section = PARSE_ENTITIES; }
			else                                  { parse_section = PARSE_INVALID; }
			continue;
		}else if(*info_start == ']'){ //entity
			parse_section = PARSE_ENTITY;
			entity_id = u32(atoi(info_start+1));
			entity = 0;
			continue;
		}else if(*info_start == ':'){ //attribute
			parse_section = PARSE_ATTRIBUTE;
			attribute_type = Type(atoi(info_start+1));
			continue;
		}
		if(parse_section == PARSE_INVALID){ ParseError("Invalid header; Skipping line."); continue; }
		
		//split the key-value pair
		key_start = info_start;
		key_end   = key_start;
		while(key_end != info_end && *key_end++ != ' ');
		if(key_end == info_end){ ParseError("No key passed."); continue; }
		key_end -= 1;
		cstring key{key_start, u64(key_end-key_start)};
		
		value_end   = info_end;
		value_start = key_end;
		while(*value_start++ == ' ');
		value_start -= 1;
		if(value_end == value_start){ ParseError("No value passed."); continue; }
		cstring value{value_start, u64(value_end-value_start)};
		
		//utility parsing funcs
		auto parse_b32 = [level_name,line_number](cstring s){
			b32 result = false;
			if     (s == cstr_lit("true"))  result = true;
			else if(s == cstr_lit("1"))     result = true;
			else if(s == cstr_lit("false")) result = false;
			else if(s == cstr_lit("0"))     result = false;
			else ParseError("Invalid boolean value: ",s);
			return result;
		};
		auto parse_vec3 = [](cstring s){
			vec3 result = vec3::ZERO;
			char* cursor;
			result.x = strtof(s.str+1, &cursor);
			result.y = strtof(cursor+1, &cursor);
			result.z = strtof(cursor+1, 0);
			return result;
		};
		
		//parse the key-value pair
		switch(parse_section){
			case PARSE_LEVEL:{
				if      (key == cstr_lit("name")){
					//TODO send level name to editor
				}else if(key == cstr_lit("last_updated")){
					//TODO check time when doing diff checking
				}else{ ParseError("Unhandled LEVEL key."); }
			}break;
			case PARSE_ENTITIES:{
				//NOTE do nothing currently
			}break;
			case PARSE_ENTITY:{
				if(key == cstr_lit("type")){
					switch(b10tou64(value)){
						case EntityType_Player:{ 
							player = new PlayerEntity;
							player->type = EntityType_Player;
							player->id = entity_id;
							entities.add(player);
							entity = player;
						}break;
						case EntityType_Physics:{ 
							PhysicsEntity* e = new PhysicsEntity;
							e->type = EntityType_Physics;
							e->id = entity_id;
							entities.add(e);
							entity = e;
						}break;
						case EntityType_Scenery:{ 
							SceneryEntity* e = new SceneryEntity;
							e->type = EntityType_Scenery;
							e->id = entity_id;
							entities.add(e);
							entity = e;
						}break;
						case EntityType_Trigger:{ 
							TriggerEntity* e = new TriggerEntity;
							e->type = EntityType_Trigger;
							e->id = entity_id;
							entities.add(e);
							triggers.add(e);
							entity = e;
						}break;
						case EntityType_Door:{ 
							DoorEntity* e = new DoorEntity;
							e->type = EntityType_Door;
							e->id = entity_id;
							entities.add(e);
							entity = e;
						}break;
						default:{ ParseError("Unhandled entity type: ",value); }break;
					}
				}else if(entity){
					if      (key == cstr_lit("name")){
						entity->name = string(value_start+1, value_end-value_start-2);
					}else if(key == cstr_lit("position")){
						entity->transform.position = parse_vec3(value);
					}else if(key == cstr_lit("rotation")){
						entity->transform.rotation = parse_vec3(value);
					}else if(key == cstr_lit("scale")){
						entity->transform.scale = parse_vec3(value);
					}else if(key == cstr_lit("attributes")){
						cstring s = value;
						while(s){ 
							switch(u32(b10tou64(s, &s))){
								case AttributeType_ModelInstance:{
									modelArr.add(ModelInstance());
									entity->model = modelArr.last;
									entity->model->attribute.entity = entity;
								}break;
								case AttributeType_Physics:{
									physicsArr.add(Physics());
									entity->physics = physicsArr.last;
									entity->physics->attribute.entity = entity;
								}break;
								case AttributeType_Player:{
									Assert(!"not implemented");
								}break;
								case AttributeType_Movement:{
									Assert(!"not implemented");
								}break;
								case AttributeType_InterpTransform:{
									interpTransformArr.add(InterpTransform());
									entity->interp = interpTransformArr.last;
									entity->interp->attribute.entity = entity;
									entity->interp->physics = entity->physics;
								}break;
							}
						}
					}else if(key == cstr_lit("connections")){
						u32 x = entity_connections.count;
						entity_connections.add({entity_id,{}});
						cstring s = value;
						while(s){ entity_connections[x].second.add(u32(b10tou64(s, &s))); }
					}else if(key == cstr_lit("events")){
						TriggerEntity* e = (TriggerEntity*)entity;
						cstring s = value;
						while(s){ e->events.add(u32(b10tou64(s, &s))); }
					}
				}else{ ParseError("Key before 'type' key in ENTITY section."); }
			}break;
			case PARSE_ATTRIBUTE:{
				switch(attribute_type){
					case AttributeType_ModelInstance:{
						if      (key == cstr_lit("model")){
							entity->model->ChangeModel(Storage::CreateModelFromFile((to_string(cstring{value_start+1,u64(value_end-value_start-2)}) + ".model").str).second);
						}else if(key == cstr_lit("visible")){
							entity->model->visible = parse_b32(value);
						}else{ ParseError("Unhandled ModelInstance key: ",key); }
					}break;
					case AttributeType_Physics:{
						if      (key == cstr_lit("position")){
							entity->physics->position = parse_vec3(value);
						}else if(key == cstr_lit("rotation")){
							entity->physics->rotation = parse_vec3(value);
						}else if(key == cstr_lit("scale")){
							entity->physics->scale = parse_vec3(value);
						}else if(key == cstr_lit("velocity")){
							entity->physics->velocity = parse_vec3(value);
						}else if(key == cstr_lit("accel")){
							entity->physics->acceleration = parse_vec3(value);
						}else if(key == cstr_lit("rot_velocity")){
							entity->physics->rotVelocity = parse_vec3(value);
						}else if(key == cstr_lit("rot_accel")){
							entity->physics->rotAcceleration = parse_vec3(value);
						}else if(key == cstr_lit("mass")){
							entity->physics->mass = (f32)atof(value_start);
						}else if(key == cstr_lit("elasticity")){
							entity->physics->elasticity = (f32)atof(value_start);
						}else if(key == cstr_lit("kinetic_fric")){
							entity->physics->kineticFricCoef = (f32)atof(value_start);
						}else if(key == cstr_lit("static_fric")){
							entity->physics->staticFricCoef = (f32)atof(value_start);
						}else if(key == cstr_lit("static_pos")){
							entity->physics->staticPosition = parse_b32(value);
						}else if(key == cstr_lit("static_rot")){
							entity->physics->staticRotation = parse_b32(value);
						}else if(key == cstr_lit("collider_shape")){
							switch(Type(b10tou64(value))){
								case ColliderShape_AABB:   { entity->physics->collider = new AABBCollider(vec3::ONE, entity->physics->mass); }break;
								case ColliderShape_Sphere: { entity->physics->collider = new SphereCollider(1.f, entity->physics->mass); }break;
								case ColliderShape_Complex:{ Assert(!"not implemented"); }break;
								default:{ ParseError("Unhandled Collider shape: ",value); }break;
							}
						}else if(key == cstr_lit("collider_offset")){
							entity->physics->collider->offset = parse_vec3(value);
						}else if(key == cstr_lit("collider_layer")){
							entity->physics->collider->layer = atoi(value_start);
						}else if(key == cstr_lit("collider_nocollide")){
							entity->physics->collider->noCollide = parse_b32(value);
						}else if(key == cstr_lit("collider_trigger")){
							entity->physics->collider->isTrigger = parse_b32(value);
						}else if(key == cstr_lit("collider_half_dims")){
							AABBCollider* c = (AABBCollider*)entity->physics->collider;
							c->halfDims = parse_vec3(value);
							c->RecalculateTensor(entity->physics->mass);
						}else if(key == cstr_lit("collider_radius")){
							SphereCollider* c = (SphereCollider*)entity->physics->collider;
							c->radius = (f32)atof(value_start);
							c->RecalculateTensor(entity->physics->mass);
						}else{ ParseError("Unhandled Physics key: ",key); }
					}break;
					case AttributeType_Player:{
						Assert(!"not implemented");
					}break;
					case AttributeType_Movement:{
						Assert(!"not implemented");
					}break;
					case AttributeType_InterpTransform:{
						if      (key == cstr_lit("type")){
							entity->interp->type = Type(b10tou64(value));
							interp_stage = 0;
						}else if(key == cstr_lit("duration")){
							entity->interp->duration = (f32)atof(value_start);
						}else if(key == cstr_lit("current")){
							entity->interp->current = (f32)atof(value_start);
						}else if(key == cstr_lit("active")){
							entity->interp->active = parse_b32(value);
						}else if(key == cstr_lit("stage_count")){
							entity->interp->stages.resize(b10tou64(value));
						}else if(key == cstr_lit("stage_position")){
							entity->interp->stages[interp_stage].position = parse_vec3(value);
						}else if(key == cstr_lit("stage_rotation")){
							entity->interp->stages[interp_stage].rotation = parse_vec3(value);
						}else if(key == cstr_lit("stage_scale")){
							entity->interp->stages[interp_stage].scale = parse_vec3(value);
							interp_stage += 1;
						}else{ ParseError("Unhandled InterpTransform key: ",key); }
					}break;
					default:{ ParseError("Unhandled attribute type: ",attribute_type); }break;
				}
			}break;
			default:{ Assert(!"unhandled section"); }break;
		}
	}
	
	//fixup connections
	forE(entity_connections){
		for(u32 id : it->second){
			entities[it->first]->connections.add(entities[id]);
		}
	}
}
#undef ParseError

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
			if(ModelInstance* mc = e->model){
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

