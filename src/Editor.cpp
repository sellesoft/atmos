#include "Editor.h"
#include "admin.h"
#include "camerainstance.h"
#include "entities/Entity.h"
#include "entities/PlayerEntity.h"
#include "entities/PhysicsEntity.h"
#include "entities/TriggerEntity.h"
#include "entities/SceneryEntity.h"
#include "entities/DoorEntity.h"
#include "attributes/ModelInstance.h"
#include "core/imgui.h"
#include "core/io.h"
#include "core/logging.h"
#include "core/assets.h"
#include "core/console.h"
#include "core/renderer.h"
#include "core/window.h"
#include "core/input.h"
#include "core/time.h"
#include "core/storage.h"
#include "utils/array.h"
#include "math/Math.h"
#include "geometry/geometry.h"
#include "geometry/Edge.h"

/////////////////////////
//// @editor structs ////
/////////////////////////
enum EditActionType_{
	EditActionType_NONE, 
	EditActionType_Translate, 
	EditActionType_Rotate, 
	EditActionType_Scale, 
	EditActionType_COUNT,
}; typedef u32 EditActionType;

struct EditAction{ //48 bytes
	EditActionType type;
	u32 data[11];
};

//////////////////////
//// @editor vars ////
//////////////////////
#include <deque>
local std::deque<EditAction> undos = std::deque<EditAction>(); //TODO(delle,Op) maybe use a vector with fixed size and store 
local std::deque<EditAction> redos = std::deque<EditAction>(); //redos at back and use swap rather than construction/deletion

local array<Entity*> selected_entities;
local array<string> dir_levels;
local array<string> dir_meshes;
local array<string> dir_textures;
local array<string> dir_materials;
local array<string> dir_models;
local array<string> dir_fonts;
local array<string> dir_objs;

local array<bool> saved_meshes({ true });
local array<bool> saved_materials({ true });
local array<bool> saved_models({ true });

local array<Entity*> copied_entities;

local bool popoutInspector;
local bool showInspector;
local bool showTimes;
local bool showDebugBar;
local bool showMenuBar;
local bool showImGuiDemoWindow;
local bool showDebugLayer;
local bool showWorldGrid;

local bool  WinHovFlag = false;
local float menubarheight = 0;
local float debugbarheight = 0;
local float padding = 0.95f;
local float fontw = 0;
local float fonth = 0;
local f32 fontsize = 0;

#define WinHovCheck if(ImGui::IsWindowHovered()) WinHovFlag = true 
#define SetPadding ImGui::SetCursorPosX((ImGui::GetWindowWidth() - (ImGui::GetWindowWidth() * padding)) / 2)

//functions to simplify the usage of our DebugLayer
namespace ImGui {
	void BeginDebugLayer(){
		//ImGui::SetNextWindowSize(ImVec2(DeshWindow->width, DeshWindow->height));
		ImGui::SetNextWindowPos(ImVec2(0, 0));
		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImGui::ColorToImVec4(color(0, 0, 0, 0)));
		ImGui::Begin("DebugLayer", 0, ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar);
	}
	
	//not necessary, but I'm adding it for clarity in code
	void EndDebugLayer(){
		ImGui::PopStyleColor();
		ImGui::End();
	}
	
	void DebugDrawText(const char* text, vec2 pos, color color = Color_White){
		ImGui::SetCursorPos(ImGui::vec2ToImVec2(pos));
		
		ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorToImVec4(color));
		ImGui::TextEx(text);
		ImGui::PopStyleColor();
	}
	
	void DebugDrawText3(const char* text, vec3 pos, color color = Color_White, vec2 twoDoffset = vec2::ZERO){
		CameraInstance* c = &AtmoAdmin->camera;
		vec2 windimen = DeshWindow->dimensions;
		
		vec3 posc = Math::WorldToCamera3(pos, c->viewMat);
		if(Math::ClipLineToZPlanes(posc, posc, c->nearZ, c->farZ)){
			ImGui::SetCursorPos(ImGui::vec2ToImVec2(Math::CameraToScreen2(posc, c->projMat, windimen) + twoDoffset));
			ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorToImVec4(color));
			ImGui::TextEx(text);
			ImGui::PopStyleColor();
		}
	}
	
	void AddPadding(float x){
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + x);
	}
} //namespace ImGui

#define BoolButton(value, tag) ImGui::Button((value) ? "True##"tag : "False##"tag, ImVec2(-FLT_MIN, 0))

///////////////
//// @undo ////
///////////////
//translate data layout:
//0x00  Transform* | transform
//0x08  vec3       | old position
//0x14  vec3       | new position
void AddUndoTranslate(Transform* t, vec3* oldPos, vec3* newPos){
	Assert(sizeof(u32)*2 == sizeof(void*), "assume ptr is 8 bytes");
	EditAction edit; edit.type = EditActionType_Translate;
	memcpy(edit.data + 0, &t,     sizeof(u32)*2);
	memcpy(edit.data + 2, oldPos, sizeof(u32)*3);
	memcpy(edit.data + 5, newPos, sizeof(u32)*3);
	undos.push_back(edit);
	redos.clear();
}
void UndoTranslate(EditAction* edit){
	Transform* t; memcpy(&t, edit->data, sizeof(u32)*2);
	t->position = vec3(((f32*)edit->data) + 2);
}
void RedoTranslate(EditAction* edit){
	Transform* t; memcpy(&t, edit->data, sizeof(u32)*2);
	t->position = vec3(((f32*)edit->data) + 5);
}

//rotate data layout:
//0x00  Transform* | transform
//0x08  vec3       | old rotation
//0x14  vec3       | new rotation
void AddUndoRotate(Transform* t, vec3* oldRot, vec3* newRot){
	Assert(sizeof(u32)*2 == sizeof(void*), "assume ptr is 8 bytes");
	EditAction edit; edit.type = EditActionType_Rotate;
	memcpy(edit.data + 0, &t,     sizeof(u32)*2);
	memcpy(edit.data + 2, oldRot, sizeof(u32)*3);
	memcpy(edit.data + 5, newRot, sizeof(u32)*3);
	undos.push_back(edit);
	redos.clear();
}
void UndoRotate(EditAction* edit){
	Transform* t; memcpy(&t, edit->data, sizeof(u32)*2);
	t->rotation = vec3(((f32*)edit->data) + 2);
}
void RedoRotate(EditAction* edit){
	Transform* t; memcpy(&t, edit->data, sizeof(u32)*2);
	t->rotation = vec3(((f32*)edit->data) + 5);
}

//scale data layout:
//0x00  Transform* | transform
//0x08  vec3       | old scale
//0x14  vec3       | new scale
void AddUndoScale(Transform* t, vec3* oldScale, vec3* newScale){
	Assert(sizeof(u32)*2 == sizeof(void*), "assume ptr is 8 bytes");
	EditAction edit; edit.type = EditActionType_Scale;
	memcpy(edit.data + 0, &t,       sizeof(u32)*2);
	memcpy(edit.data + 2, oldScale, sizeof(u32)*3);
	memcpy(edit.data + 5, newScale, sizeof(u32)*3);
	undos.push_back(edit);
	redos.clear();
}
void UndoScale(EditAction* edit){
	Transform* t; memcpy(&t, edit->data, sizeof(u32)*2);
	t->scale = vec3(((f32*)edit->data) + 2);
}
void RedoScale(EditAction* edit){
	Transform* t; memcpy(&t, edit->data, sizeof(u32)*2);
	t->scale = vec3(((f32*)edit->data) + 5);
}

void Undo(u32 count = 1){
	forI((count < undos.size()) ? count : undos.size()){
		u32 n = undos.size()-i-1;
		switch(undos[n].type){
			case(EditActionType_Translate):{ UndoTranslate(&undos[n]); }break;
			case(EditActionType_Rotate):   { UndoRotate(&undos[n]);    }break;
			case(EditActionType_Scale):    { UndoScale(&undos[n]);     }break;
		}
		redos.push_back(undos.back());
		undos.pop_back();
	}
}

void Redo(u32 count = 1){
	forI((count < redos.size()) ? count : redos.size()){
		u32 n = redos.size()-i-1;
		switch(redos[n].type){
			case(EditActionType_Translate):{ RedoTranslate(&redos[n]); }break;
			case(EditActionType_Rotate):   { RedoRotate(&redos[n]);    }break;
			case(EditActionType_Scale):    { RedoScale(&redos[n]);     }break;
		}
		undos.push_back(redos.back());
		redos.pop_back();
	}
}


///////////////
//// @menu ////
///////////////
void MenuBar(){
	ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 0);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
	ImGui::PushStyleColor(ImGuiCol_PopupBg, ImGui::ColorToImVec4(color(20, 20, 20, 255)));
	ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImGui::ColorToImVec4(color(20, 20, 20, 255)));
	
	if(ImGui::BeginMainMenuBar()){ WinHovCheck;
		menubarheight = ImGui::GetWindowHeight();
		
		//// level menu options ////
		if(ImGui::BeginMenu("Level")){ WinHovCheck;
			if(ImGui::MenuItem("New")){
				AtmoAdmin->Reset();
			}
			if(ImGui::MenuItem("Save")){
				if(AtmoAdmin->levelName == ""){
					LogW("editor","Level not saved before; Use 'Save As'");
				}else{;
					AtmoAdmin->SaveLevel(cstring{AtmoAdmin->levelName.str,(u64)AtmoAdmin->levelName.count});
				}
			}
			if(ImGui::BeginMenu("Save As")){ WinHovCheck;
				persist char buff[255] = {};
				if(ImGui::InputText("##level_saveas_input", buff, 255, ImGuiInputTextFlags_EnterReturnsTrue)){
					AtmoAdmin->levelName = string(buff);
					AtmoAdmin->SaveLevel(cstring{AtmoAdmin->levelName.str,(u64)AtmoAdmin->levelName.count});
				}
				ImGui::EndMenu();
			}
			if(ImGui::BeginMenu("Load")){ WinHovCheck;
				array<File> levels = get_directory_files(Assets::dirLevels().c_str());
				forE(levels){
					string name(it->name, it->short_length);
					if(!it->is_directory && ImGui::MenuItem(name.str)){
						AtmoAdmin->LoadLevel({it->name,it->short_length});
					}
				}
				ImGui::EndMenu();
			}
			ImGui::EndMenu();
		}
		
		//// load menu options ////
		if(ImGui::BeginMenu("Load")){ WinHovCheck;
			if(ImGui::BeginMenu("Meshes")){ WinHovCheck;
				dir_meshes = Assets::iterateDirectory(Assets::dirModels(), ".mesh");
				forX(di, dir_meshes.count){
					bool loaded = false;
					forX(li, Storage::MeshCount()){
						if(strncmp(Storage::MeshName(li), dir_meshes[di].str, dir_meshes[di].count - 5) == 0){
							loaded = true;  break;
						}
					}
					if(!loaded && ImGui::MenuItem(dir_meshes[di].str)){ WinHovCheck;
						u32 id = Storage::CreateMeshFromFile(dir_meshes[di].str).first;
						if(id) saved_meshes.add(true);
					}
				}
				ImGui::EndMenu();
			}
			
			if(ImGui::BeginMenu("Textures")){ WinHovCheck;
				dir_textures = Assets::iterateDirectory(Assets::dirTextures());
				forX(di, dir_textures.count){
					bool loaded = false;
					forX(li, Storage::TextureCount()){
						if(strcmp(Storage::TextureName(li), dir_textures[di].str) == 0){
							loaded = true;  break;
						}
					}
					if(!loaded && ImGui::MenuItem(dir_textures[di].str)){ WinHovCheck;
						Storage::CreateTextureFromFile(dir_textures[di].str);
					}
				}
				ImGui::EndMenu();
			}
			
			if(ImGui::BeginMenu("Materials")){ WinHovCheck;
				dir_materials = Assets::iterateDirectory(Assets::dirModels(), ".mat");
				forX(di, dir_materials.count){
					bool loaded = false;
					forX(li, Storage::MaterialCount()){
						if(strncmp(Storage::MaterialName(li), dir_materials[di].str, dir_materials[di].count - 4) == 0){
							loaded = true;  break;
						}
					}
					if(!loaded && ImGui::MenuItem(dir_materials[di].str)){ WinHovCheck;
						u32 id = Storage::CreateMaterialFromFile(dir_materials[di].str).first;
						if(id) saved_materials.add(true);
					}
				}
				ImGui::EndMenu();
			}
			
			if(ImGui::BeginMenu("Models")){ WinHovCheck;
				dir_models = Assets::iterateDirectory(Assets::dirModels(), ".model");
				forX(di, dir_models.count){
					bool loaded = false;
					forX(li, Storage::ModelCount()){
						if(strncmp(Storage::ModelName(li), dir_models[di].str, dir_models[di].count - 6) == 0){
							loaded = true;  break;
						}
					}
					if(!loaded && ImGui::MenuItem(dir_models[di].str)){ WinHovCheck;
						u32 mesh_count = Storage::MeshCount();
						u32 material_count = Storage::MaterialCount();
						u32 id = Storage::CreateModelFromFile(dir_models[di].str, ModelFlags_NONE, false).first;
						if(id) saved_models.add(true);
						forI(Storage::MeshCount() - mesh_count){ saved_meshes.add(true); }
						forI(Storage::MaterialCount() - material_count){ saved_materials.add(true); }
					}
				}
				ImGui::EndMenu();
			}
			
			if(ImGui::BeginMenu("OBJs")){ WinHovCheck;
				dir_objs = Assets::iterateDirectory(Assets::dirModels(), ".obj");
				forX(di, dir_objs.count){
					u32 loaded_idx = -1;
					forX(li, Storage::ModelCount()){
						if(strcmp(Storage::ModelName(li), dir_objs[di].str) == 0){
							loaded_idx = li;  break;
						}
					}
					if(ImGui::MenuItem(dir_objs[di].str)){ WinHovCheck;
						if(loaded_idx != -1){
							Storage::DeleteModel(loaded_idx);
							saved_models.remove(loaded_idx);
						}
						u32 mesh_count = Storage::MeshCount();
						u32 material_count = Storage::MaterialCount();
						
						u32 id = Storage::CreateModelFromFile(dir_objs[di].str, ModelFlags_NONE, true).first;
						if(id) saved_models.add(false);
						
						forI(Storage::MeshCount() - mesh_count){ saved_meshes.add(false); }
						forI(Storage::MaterialCount() - material_count){ saved_materials.add(false); }
					}
				}
				ImGui::EndMenu();
			}
			ImGui::EndMenu();
		}
		
		//// window menu options ////
		if(ImGui::BeginMenu("Window")){ WinHovCheck;
			ImGui::Checkbox("Inspector", &showInspector);
			ImGui::Checkbox("Debug Bar", &showDebugBar);
			ImGui::Checkbox("DebugLayer", &showDebugLayer);
			ImGui::Checkbox("Timers", &showTimes);
			ImGui::Checkbox("World Grid", &showWorldGrid);
			ImGui::Checkbox("ImGui Demo", &showImGuiDemoWindow);
			ImGui::Checkbox("Popout Inspector", &popoutInspector);
			ImGui::EndMenu();
		}
		
		//// state menu options ////
		if(ImGui::BeginMenu("State")){ WinHovCheck;
			if(ImGui::MenuItem("Play"))   AtmoAdmin->ChangeState(GameState_Play);
			if(ImGui::MenuItem("Editor")) AtmoAdmin->ChangeState(GameState_Editor);
			if(ImGui::MenuItem("Menu"))   AtmoAdmin->ChangeState(GameState_Menu);
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}
	
	ImGui::PopStyleColor(2);
	ImGui::PopStyleVar(2);
}

///////////////////
//// @entities ////
///////////////////
local void CutEntities(){
	//!Incomplete
	Assert(!"not implemented");
	copied_entities.clear();
}

local void CopyEntities(){
	copied_entities.clear();
	copied_entities.add(selected_entities);
}

local void PasteEntities(){
	selected_entities.clear();
	for(Entity* src : copied_entities){
		Entity* dst = 0;
		switch(src->type){
			case EntityType_Player:{
				LogE("editor","There can only be one player entity.");
				continue;
			}break;
			case EntityType_Physics:{
				dst = new PhysicsEntity;
			}break;
			case EntityType_Scenery:{
				dst = new SceneryEntity;
			}break;
			case EntityType_Trigger:{
				TriggerEntity* e = new TriggerEntity;
				e->events = ((TriggerEntity*)src)->events;
				dst = e;
			}break;
			case EntityType_Door:{
				dst = new DoorEntity;
			}break;
			default:{
				LogfE("editor","Unhandled entity type '%d' when pasting copied entities.",src->type);
				continue;
			}break;
		}
		
		dst->id = AtmoAdmin->entities.count;
		dst->type = src->type;
		dst->name = src->name.str;
		memcpy(&dst->transform, &src->transform, sizeof(Transform));
		dst->connections = src->connections;
		if(src->model){
			AtmoAdmin->modelArr.add(ModelInstance(src->model->model));
			dst->model = AtmoAdmin->modelArr.last;
			dst->model->attribute.entity = dst;
			dst->model->visible = src->model->visible;
		}
		if(src->physics){
			AtmoAdmin->physicsArr.add(Physics());
			dst->physics = AtmoAdmin->physicsArr.last;
			memcpy(dst->physics, src->physics, sizeof(Physics));
			dst->physics->attribute.entity = dst;
			
			switch(src->physics->collider.type){
				case ColliderType_NONE:{
					//nothing special
				}break;
				case ColliderType_AABB:{
					dst->physics->collider = AABBCollider(src->physics->collider.halfDims, src->physics->mass);
				}break;
				case ColliderType_Sphere:{
					dst->physics->collider = SphereCollider(src->physics->collider.radius, src->physics->mass);
				}break;
				default:{
					LogfE("editor","Unhandled collider type '%d' when pasting copied entities.",src->physics->collider.type);
				}break;
			}
			dst->physics->collider.type = src->physics->collider.type;
			dst->physics->collider.tensor = src->physics->collider.tensor;
			dst->physics->collider.offset = src->physics->collider.offset;
			dst->physics->collider.noCollide = src->physics->collider.noCollide;
			dst->physics->collider.isTrigger = src->physics->collider.isTrigger;
			dst->physics->collider.triggerActive = src->physics->collider.triggerActive;
			dst->physics->collider.playerOnly = src->physics->collider.playerOnly;
		}
		if(src->interp){
			AtmoAdmin->interpTransformArr.add(InterpTransform());
			dst->interp = AtmoAdmin->interpTransformArr.last;
			dst->interp->attribute.entity = dst;
			dst->interp->physics = dst->physics;
			dst->interp->type = src->interp->type;
			dst->interp->duration = src->interp->duration;
			dst->interp->current = src->interp->current;
			dst->interp->active = src->interp->active;
			dst->interp->reset = src->interp->reset;
			dst->interp->stages = src->interp->stages;
		}
		
		AtmoAdmin->entities.add(dst);
		selected_entities.add(dst);
	}
}

void EntitiesTab(){
	persist bool rename_ent = false;
	persist char rename_buffer[DESHI_NAME_SIZE] = {};
	persist Entity* events_ent = 0;
	
	//// selected entity keybinds ////
	//start renaming first selected entity
	//TODO(delle) repair this to work with array
	if(selected_entities.count && DeshInput->KeyPressed(Key::F2)){
		rename_ent = true;
		DeshConsole->IMGUI_KEY_CAPTURE = true;
		//if(selected_entities.size > 1) selected_entities.remove(selected_entities.end());
		memcpy(rename_buffer, selected_entities[0]->name.str, selected_entities[0]->name.count*sizeof(char));
	}
	//submit renaming entity
	if(rename_ent && DeshInput->KeyPressed(Key::ENTER)){
		rename_ent = false;
		DeshConsole->IMGUI_KEY_CAPTURE = false;
		selected_entities[0]->name = rename_buffer;
	}
	//stop renaming entity
	if(rename_ent && DeshInput->KeyPressed(Key::ESCAPE)){
		rename_ent = false;
		DeshConsole->IMGUI_KEY_CAPTURE = false;
	}
	//delete selected entities
	if(selected_entities.count && DeshInput->KeyPressed(Key::DELETE)){
		//TODO(delle) re-enable this with a popup to delete OR with undoing on delete
		selected_entities.clear();
	}
	
	//// entity list panel ////
	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::ColorToImVec4(color(25, 25, 25)));
	ImGui::SetCursorPosX(ImGui::GetWindowWidth() * 0.025);
	if(ImGui::BeginChild("##entity_list", ImVec2(ImGui::GetWindowWidth() * 0.95f, 100))){
		//if no entities, draw empty list
		if(AtmoAdmin->entities.count == 0){
			float time = DeshTime->totalTime;
			cstring str1 = cstring_lit("Nothing yet...");
			float strlen1 = fontw * str1.count;
			for(int i = 0; i < str1.count; i++){
				ImGui::SetCursorPos(ImVec2((ImGui::GetWindowSize().x - strlen1) / 2 + i * (fontsize / 2), (ImGui::GetWindowSize().y - fontsize) / 2 + sin(10 * time + cos(10 * time + (i * M_PI / 2)) + (i * M_PI / 2))));
				ImGui::TextEx(str1.str+i, str1.str+i+1);
			}
		}else{
			if(ImGui::BeginTable("##entity_list_table", 4, ImGuiTableFlags_BordersInner)){
				ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, fontw * 3.5f); //visible button
				ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, fontw * 5.f);  //id
				ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch);             //name
				ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, fontw);        //delete button
				
				forX(ent_idx, AtmoAdmin->entities.count){
					Entity* ent = AtmoAdmin->entities[ent_idx];
					if(!ent) Assert(!"NULL entity when creating entity list table");
					ImGui::PushID(ent_idx);
					ImGui::TableNextRow();
					
					//// visible button ////
					ImGui::TableSetColumnIndex(0);
					if(ModelInstance* m = ent->model){
						if(ImGui::Button((m->visible) ? "(M)" : "( )", ImVec2(-FLT_MIN, 0.0f))){
							m->ToggleVisibility();
						}
					}else{
						if(ImGui::Button("(?)", ImVec2(-FLT_MIN, 0.0f))){}
					}
					
					//// id + label (selectable row) ////
					ImGui::TableSetColumnIndex(1);
					char label[16];
					sprintf(label, "%04d ", ent_idx);
					u32 selected_idx = -1;
					forI(selected_entities.count){ if(ent == selected_entities[i]){ selected_idx = i; break; } }
					bool is_selected = selected_idx != -1;
					if(ImGui::Selectable(label, is_selected, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap)){
						if(is_selected){
							if(DeshInput->LCtrlDown()){
								selected_entities.remove(selected_idx);
							}else{
								selected_entities.clear();
								selected_entities.add(ent);
							}
						}else{
							if(DeshInput->LCtrlDown()){
								selected_entities.add(ent);
							}else{
								selected_entities.clear();
								selected_entities.add(ent);
							}
						}
						rename_ent = false;
						DeshConsole->IMGUI_KEY_CAPTURE = false;
					}
					
					//// name text ////
					ImGui::TableSetColumnIndex(2);
					if(rename_ent && selected_idx == ent_idx){
						ImGui::PushStyleColor(ImGuiCol_FrameBg, ImColor(0xff203c56).Value);
						ImGui::InputText("##ent_rename_input", rename_buffer, DESHI_NAME_SIZE, ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue);
						ImGui::PopStyleColor();
					}else{
						ImGui::TextEx(ent->name.str);
					}
					
					//// delete button ////
					ImGui::TableSetColumnIndex(3);
					if(ImGui::Button("X", ImVec2(-FLT_MIN, 0.0f))){
						if(is_selected) selected_entities.remove(selected_idx);
						//AtmoAdmin->DeleteEntity(ent);
					}
					ImGui::PopID();
				}
				ImGui::EndTable();
			}
		}
	}ImGui::EndChild(); //entity_list
	ImGui::PopStyleColor();
	
	ImGui::Separator();
	
	//// create new entity ////
	persist const char* presets[] = { "Empty", "Static", "Physics", "Player", "Visible Trigger", "Invisible Trigger", "Door", "Scenery"};
	persist int current_preset = 0;
	
	ImGui::SetCursorPosX(ImGui::GetWindowWidth() * 0.025);
	if(ImGui::Button("New Entity")){
		Entity* ent = 0;
		
		string ent_name = TOSTRING(presets[current_preset], AtmoAdmin->entities.count);
		switch (current_preset){
			case(0):default:{ //Empty
				LogE("editor","Creating empty entities not setup yet.");
			}break;
			case(1):        { //StaticEntity
				PhysicsEntity* e = new PhysicsEntity;
				e->Init(ent_name.str, Transform(), Storage::NullModel(), AABBCollider(Storage::NullMesh(),1.0f),1.0f,true);
				ent = e;
			}break;
			case(2):        { //PhysicsEntity
				PhysicsEntity* e = new PhysicsEntity;
				e->Init(ent_name.str, Transform(), Storage::NullModel(), AABBCollider(Storage::NullMesh(),1.0f),1.0f);
				ent = e;
			}break;
			case(3):        { //Player
				if(!AtmoAdmin->player){
					AtmoAdmin->player = new PlayerEntity;
					AtmoAdmin->player->Init();
					ent = AtmoAdmin->player;
				}
			}break;
			case(4):        { //Visible Trigger
				TriggerEntity* e = new TriggerEntity;
				e->Init(ent_name.str, Transform(), AABBCollider(Storage::NullMesh(),1.0f), Storage::NullModel());
				ent = e;
			}break;
			case(5):        { //Invisible Trigger
				TriggerEntity* e = new TriggerEntity;
				e->Init(ent_name.str, Transform(), AABBCollider(Storage::NullMesh(),1.0f));
				ent = e;
			}break;
			case(6):        { //Door
				DoorEntity* e = new DoorEntity;
				e->Init(ent_name.str, AABBCollider(Storage::NullMesh(),1.0f), Storage::NullModel(), Transform(), Transform(), 1.f);
				ent = e;
			}break;
			case(7):        { //Scenery
				SceneryEntity* e = new SceneryEntity;
				e->Init(ent_name.str, Transform(), Storage::NullModel());
				ent = e;
			}break;
		}
		
		selected_entities.clear();
		if(ent) selected_entities.add(ent);
	}
	ImGui::SameLine(); ImGui::Combo("##preset_combo", &current_preset, presets, ArrayCount(presets));
	
	ImGui::Separator();
	
	//// selected entity inspector panel ////
	Entity* sel = selected_entities.count ? selected_entities[0] : 0;
	if(!sel) return;
	ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 5.0f);
	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::ColorToImVec4(color(25, 25, 25)));
	ImGui::SetCursorPosX(ImGui::GetWindowWidth() * 0.025);
	if(ImGui::BeginChild("##ent_inspector", ImVec2(ImGui::GetWindowWidth() * 0.95f, ImGui::GetWindowHeight()*.9f - 100), true, ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoCollapse)){
		//// name ////
		SetPadding; ImGui::TextEx("Name:"); ImGui::SameLine(); ImGui::SetNextItemWidth(-FLT_MIN);
		ImGui::InputText("##ent_name_input", sel->name.str, DESHI_NAME_SIZE, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);
		
		//// transform ////
		int tree_flags = ImGuiTreeNodeFlags_DefaultOpen;
		if(ImGui::CollapsingHeader("Transform", 0, tree_flags)){
		    ImGui::Indent();
			vec3 oldVec = sel->transform.position;
			
			ImGui::TextEx("Position    "); ImGui::SameLine();
			if(ImGui::Inputvec3("##ent_pos", &sel->transform.position)){
				if(sel->physics){
					sel->physics->position = sel->transform.position;
					AddUndoTranslate(&sel->transform, &oldVec, &sel->physics->position);
				}else{
					AddUndoTranslate(&sel->transform, &oldVec, &sel->transform.position);
				}
			}ImGui::Separator();
			
			oldVec = sel->transform.rotation;
			ImGui::TextEx("Rotation    "); ImGui::SameLine();
			if(ImGui::Inputvec3("##ent_rot", &sel->transform.rotation)){
				if(sel->physics){
					sel->physics->rotation = sel->transform.rotation;
					AddUndoRotate(&sel->transform, &oldVec, &sel->physics->rotation);
				}else{
					AddUndoRotate(&sel->transform, &oldVec, &sel->transform.rotation);
				}
			}ImGui::Separator();
			
			oldVec = sel->transform.scale;
			ImGui::TextEx("Scale       "); ImGui::SameLine();
			if(ImGui::Inputvec3("##ent_scale", &sel->transform.scale)){
				if(sel->physics){
					sel->physics->scale = sel->transform.scale;
					AddUndoScale(&sel->transform, &oldVec, &sel->physics->scale);
				}else{
					AddUndoScale(&sel->transform, &oldVec, &sel->transform.scale);
				}
			}ImGui::Separator();
			ImGui::Unindent();
		}
		
		//// connections ////
		if(ImGui::CollapsingHeader("Connections", 0, tree_flags)){
			ImGui::Indent();
			
			if(sel->connections.count){
				if(ImGui::BeginTable("##ent_conn_table", 3, ImGuiTableFlags_None, ImVec2(-FLT_MIN, ImGui::GetWindowHeight() * .05f))){
					ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, fonth * 2.5f);
					ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch);
					ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, fonth);
					
					forX(conn_idx, sel->connections.count){
						ImGui::PushID(&sel->connections[conn_idx]);
						ImGui::TableNextRow();
						
						//// id ////
						ImGui::TableSetColumnIndex(0);
						ImGui::Text(" %02d", conn_idx);
						
						//// name text ////
						ImGui::TableSetColumnIndex(1);
						ImGui::TextEx(sel->connections[conn_idx]->name.str);
						
						//// delete button ////
						ImGui::TableSetColumnIndex(2);
						if(ImGui::Button("X", ImVec2(-FLT_MIN, 0.0f))){
							sel->connections.remove(conn_idx);
							ImGui::PopID();
							break;
						}
						ImGui::PopID();
					}
					ImGui::EndTable(); //ent_conn_table
				}
			}
			
			persist u32 sel_conn_entity = 0;
			if(ImGui::Button("Add Connection")) sel->connections.add(AtmoAdmin->entities[sel_conn_entity]);
			ImGui::SameLine(); ImGui::SetNextItemWidth(-1);
			if(ImGui::BeginCombo("##ent_conn_combo", AtmoAdmin->entities[sel_conn_entity]->name.str)){
				forI(AtmoAdmin->entities.count){ if(ImGui::Selectable(AtmoAdmin->entities[i]->name.str)){ sel_conn_entity = i; } }
				ImGui::EndCombo();
			}
			
			ImGui::Unindent();
			ImGui::Separator();
		}
		
		//// entity specializations ////
		if(sel->type == EntityType_Trigger && ImGui::CollapsingHeader("Trigger", 0, tree_flags)){
			ImGui::Indent();
			
			TriggerEntity* trigger = (TriggerEntity*)sel;
			if(trigger->events.count){
				if(ImGui::BeginTable("##ent_trig_table", 3, ImGuiTableFlags_None, ImVec2(-FLT_MIN, ImGui::GetWindowHeight() * .05f))){
					ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, fonth * 2.5f);
					ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch);
					ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, fonth);
					
					forX(trig_idx, trigger->events.count){
						ImGui::PushID(&trigger->events[trig_idx]);
						ImGui::TableNextRow();
						
						//// id ////
						ImGui::TableSetColumnIndex(0);
						ImGui::Text(" %02d", trig_idx);
						
						//// name text ////
						ImGui::TableSetColumnIndex(1);
						ImGui::TextEx(EventStrings[trigger->events[trig_idx]]);
						
						//// delete button ////
						ImGui::TableSetColumnIndex(2);
						if(ImGui::Button("X", ImVec2(-FLT_MIN, 0.0f))){
							trigger->events.remove(trig_idx);
							ImGui::PopID();
							break;
						}
						ImGui::PopID();
					}
					ImGui::EndTable(); //ent_conn_table
				}
			}
			
			persist Type sel_trig_event = 0;
			if(ImGui::Button("Add Event")) trigger->events.add(sel_trig_event);
			ImGui::SameLine(); ImGui::SetNextItemWidth(-1);
			if(ImGui::BeginCombo("##ent_trig_combo", EventStrings[sel_trig_event])){
				forI(Event_COUNT){ if(ImGui::Selectable(EventStrings[i])){ sel_trig_event = i; } }
				ImGui::EndCombo();
			}
			
			ImGui::Unindent();
			ImGui::Separator();
		}
		
		//// components ////
		//mesh
		if(sel->model){
			bool delete_button = 1;
			if(ImGui::CollapsingHeader("Model", &delete_button, tree_flags)){
				ImGui::Indent();
				
				ImGui::TextEx("Visible  "); ImGui::SameLine();
				if(BoolButton(sel->model->visible, "model_vis")){
					sel->model->ToggleVisibility();
				}
				
				ImGui::TextEx("Model     "); ImGui::SameLine(); ImGui::SetNextItemWidth(-1);
				if(ImGui::BeginCombo("##model_combo", sel->model->model->name)){
					forI(Storage::ModelCount()){
						if(ImGui::Selectable(Storage::ModelName(i), sel->model->model == Storage::ModelAt(i))){
							sel->model->ChangeModel(Storage::ModelAt(i));
						}
					}
					ImGui::EndCombo();
				}
				
				ImGui::Unindent();
				ImGui::Separator();
			}
			if(delete_button){
				
			}
		}
		
		//physics
		if(sel->physics){
			switch(sel->physics->collider.type){
				case ColliderType_AABB:{
					Render::DrawBox(Transform(sel->physics->position+sel->physics->collider.offset, vec3::ZERO,
											  sel->physics->scale*(sel->physics->collider.halfDims*2)).Matrix(), Color_Green);
				}break;
				case ColliderType_Sphere:{
					Render::DrawSphere(sel->physics->position+sel->physics->collider.offset, vec3::ZERO, sel->physics->collider.radius, 16, Color_Green);
				}break;
				case ColliderType_Hull:{
					mat4 transform = mat4::TransformationMatrix(sel->physics->position + sel->physics->collider.offset, sel->physics->rotation, sel->physics->scale);
					forE(sel->physics->collider.mesh->faces){
						vec3 prev = sel->physics->collider.mesh->vertexes[it->outerVertexes[0]].pos * transform;
						Render::DrawLine(prev, sel->physics->collider.mesh->vertexes[it->outerVertexes[it->outerVertexCount-1]].pos * transform, Color_Green);
						for(u32 i=1; i < it->outerVertexCount; ++i){
							vec3 curr = sel->physics->collider.mesh->vertexes[it->outerVertexes[i]].pos * transform;
							Render::DrawLine(prev, curr, Color_Green);
							prev = curr;
						}
					}
				}break;
			}
			
			bool delete_button = 1;
			if(ImGui::CollapsingHeader("Physics", &delete_button, tree_flags)){
				ImGui::Indent();
				
				ImGui::TextEx("Velocity    "); ImGui::SameLine();
				if(ImGui::Inputvec3("##phys_vel", &sel->physics->velocity));
				ImGui::TextEx("Acceleration"); ImGui::SameLine();
				if(ImGui::Inputvec3("##phys_accel", &sel->physics->acceleration));
				ImGui::TextEx("Rot Velocity"); ImGui::SameLine();
				if(ImGui::Inputvec3("##phys_rotvel", &sel->physics->rotVelocity));
				ImGui::TextEx("Rot Accel   "); ImGui::SameLine();
				if(ImGui::Inputvec3("##phys_rotaccel", &sel->physics->rotAcceleration));
				
				ImGui::TextEx("Mass            "); ImGui::SameLine(); ImGui::SetNextItemWidth(-FLT_MIN);
				ImGui::InputFloat("##phys_mass", &sel->physics->mass, 0, 0);
				ImGui::TextEx("Elasticity      "); ImGui::SameLine(); ImGui::SetNextItemWidth(-FLT_MIN);
				ImGui::InputFloat("##phys_elas", &sel->physics->elasticity, 0, 0);
				ImGui::TextEx("Kinetic Friction"); ImGui::SameLine(); ImGui::SetNextItemWidth(-FLT_MIN);
				ImGui::InputFloat("##phys_kinfric", &sel->physics->kineticFricCoef, 0, 0);
				ImGui::TextEx("Static Friction "); ImGui::SameLine(); ImGui::SetNextItemWidth(-FLT_MIN);
				ImGui::InputFloat("##phys_stafric", &sel->physics->staticFricCoef, 0, 0);
				ImGui::TextEx("Air Friction    "); ImGui::SameLine(); ImGui::SetNextItemWidth(-FLT_MIN);
				ImGui::InputFloat("##phys_airfric", &sel->physics->airFricCoef, 0, 0);
				
				ImGui::TextEx("Static Position "); ImGui::SameLine();
				if(BoolButton(sel->physics->staticPosition, "phys_stapos")){
					sel->physics->staticPosition = !sel->physics->staticPosition;
				}
				ImGui::TextEx("Static Rotation "); ImGui::SameLine();
				if(BoolButton(sel->physics->staticRotation, "phys_starot")){
					sel->physics->staticRotation = !sel->physics->staticRotation;
				}
				
				ImGui::Separator();
				
				ImGui::TextEx("Collider"); ImGui::SameLine(); ImGui::SetNextItemWidth(-1);
				if(sel->physics->collider.type != ColliderType_NONE){
					if(ImGui::BeginCombo("##phys_collider", ColliderTypeStrings[sel->physics->collider.type])){
						forI(ColliderType_COUNT){
							if(ImGui::Selectable(ColliderTypeStrings[i], sel->physics->collider.type == i)){
								switch(i){
									case ColliderType_AABB:{
										sel->physics->collider = AABBCollider(vec3{.5f,.5f,.5f},sel->physics->mass);
									}break;
									case ColliderType_Sphere:{
										sel->physics->collider = SphereCollider(1.f,sel->physics->mass);
									}break;
									case ColliderType_Hull:{
										sel->physics->collider = HullCollider(Storage::NullMesh(),sel->physics->mass);
									}break;
									case ColliderType_NONE:default:{
										sel->physics->collider.type = ColliderType_NONE;
									}break;
								}
								break;
							}
						}
						ImGui::EndCombo();
					}
					
					ImGui::TextEx("Offset     "); ImGui::SameLine();
					if(ImGui::Inputvec3("##phys_offset", &sel->physics->collider.offset));
					ImGui::TextEx("Layer     "); ImGui::SameLine(); ImGui::SetNextItemWidth(-1);
					if(ImGui::DragInt("##phys_layer", (int*)&sel->physics->collider.layer, 1, 0, INT_MAX));
					ImGui::TextEx("No Collide "); ImGui::SameLine();
					if(BoolButton(sel->physics->collider.noCollide, "phys_nocoll")){
						sel->physics->collider.noCollide = !sel->physics->collider.noCollide;
					}
					ImGui::TextEx("Is Trigger "); ImGui::SameLine();
					if(BoolButton(sel->physics->collider.isTrigger, "phys_trigger")){
						sel->physics->collider.isTrigger = !sel->physics->collider.isTrigger;
					}
					ImGui::TextEx("Player Only"); ImGui::SameLine();
					if(BoolButton(sel->physics->collider.playerOnly, "phys_player")){
						sel->physics->collider.playerOnly = !sel->physics->collider.playerOnly;
					}
					
					switch(sel->physics->collider.type){
						case ColliderType_AABB:{
							ImGui::TextEx("Half Dims  "); ImGui::SameLine();
							if(ImGui::Inputvec3("##phys_halfdims", &sel->physics->collider.halfDims)){
								sel->physics->collider.RecalculateTensor(sel->physics->mass);
							}
						}break;
						case ColliderType_Sphere:{
							ImGui::TextEx("Radius     "); ImGui::SameLine(); ImGui::SetNextItemWidth(-FLT_MIN);
							if(ImGui::InputFloat("##phys_radius", &sel->physics->collider.radius, 0, 0)){
								sel->physics->collider.RecalculateTensor(sel->physics->mass);
							}
						}break;
						case ColliderType_Hull:{
							ImGui::TextEx("Mesh       "); ImGui::SameLine(); ImGui::SetNextItemWidth(-FLT_MIN);
							if(ImGui::BeginCombo("##phys_mesh", sel->physics->collider.mesh->name)){
								forI(Storage::MeshCount()){
									if(ImGui::Selectable(Storage::MeshName(i), sel->physics->collider.mesh == Storage::MeshAt(i))){
										sel->physics->collider.mesh = Storage::MeshAt(i);
										sel->physics->collider.RecalculateTensor(sel->physics->mass);
									}
								}
								ImGui::EndCombo();
							}
						}break;
						default:{
							ImGui::TextEx("unhandled collider shape");
						}break;
					}
				}else{
					if(ImGui::BeginCombo("##phys_collider", "None")){
						forI(ColliderType_COUNT){
							if(ImGui::Selectable(ColliderTypeStrings[i], false)){
								switch(i){
									case ColliderType_AABB:{
										sel->physics->collider = AABBCollider(vec3{.5f,.5f,.5f},sel->physics->mass);
									}break;
									case ColliderType_Sphere:{
										sel->physics->collider = SphereCollider(1.f,sel->physics->mass);
									}break;
								}
							}
						}
						ImGui::EndCombo();
					}
				}
				
				ImGui::Unindent();
				ImGui::Separator();
			}
			if(delete_button){
				
			}
		}
		
		//interp transform
		if(sel->interp){
			bool delete_button = 1;
			if(ImGui::CollapsingHeader("InterpTransform", &delete_button, tree_flags)){
				ImGui::Indent();
				
				ImGui::TextEx("Type     "); ImGui::SameLine(); ImGui::SetNextItemWidth(-1);
				if(ImGui::BeginCombo("##interp_combo", InterpTransformTypeStrings[sel->interp->type])){
					forI(InterpTransformType_COUNT){
						if(ImGui::Selectable(InterpTransformTypeStrings[i], sel->interp->type == i)){
							sel->interp->type = i;
						}
					}
					ImGui::EndCombo();
				}
				ImGui::TextEx("Duration "); ImGui::SameLine(); ImGui::SetNextItemWidth(-1);
				ImGui::InputFloat("##interp_duration", &sel->interp->duration, 0, 0);
				ImGui::TextEx("Starting "); ImGui::SameLine(); ImGui::SetNextItemWidth(-1);
				ImGui::InputFloat("##interp_current", &sel->interp->current, 0, 0);
				ImGui::TextEx("Active   "); ImGui::SameLine();
				if(BoolButton(sel->interp->active, "interp_active")){
					sel->interp->active = !sel->interp->active;
				}
				
				ImGui::Separator();
				
				ImGui::TextEx("Stage Count"); ImGui::SameLine(); ImGui::SetNextItemWidth(-1);
				int stage_count = sel->interp->stages.count;
				if(ImGui::DragInt("##interp_stages", &stage_count, 1, 2, INT_MAX, "%d", ImGuiSliderFlags_AlwaysClamp)){
					if(stage_count > sel->interp->stages.count){
						sel->interp->stages.add(sel->interp->stages[sel->interp->stages.count-1]);
					}else{
						sel->interp->stages.pop(sel->interp->stages.count - stage_count);
					}
				}
				forI(sel->interp->stages.count){
					ImGui::PushID(&sel->interp->stages[i]);
					ImGui::Text("Position %d",i); ImGui::SameLine(); if(ImGui::Inputvec3("##interp_pos", &sel->interp->stages[i].position));
					ImGui::Text("Rotation %d",i); ImGui::SameLine(); if(ImGui::Inputvec3("##interp_rot", &sel->interp->stages[i].rotation));
					ImGui::Text("Scale    %d",i); ImGui::SameLine(); if(ImGui::Inputvec3("##interp_sca", &sel->interp->stages[i].scale));
					ImGui::PopID();
				}
				
				ImGui::Unindent();
				ImGui::Separator();
			}
			if(delete_button){
				
			}
		}
		
		//// add component ////
		persist int add_attr_index = 0;
		
		ImGui::SetCursorPosX(ImGui::GetWindowWidth() * 0.025);
		if(ImGui::Button("Add Attribute")){
			LogE("editor","Add Attribute not implemented yet");
		}
		ImGui::SameLine(); ImGui::SetNextItemWidth(-1);
		ImGui::Combo("##add_attr_combo", &add_attr_index, AttributeTypeStrings, ArrayCount(AttributeTypeStrings));
		
	}ImGui::EndChild(); //CreateMenu
	ImGui::PopStyleColor();
	ImGui::PopStyleVar(); //ImGuiStyleVar_IndentSpacing
} //EntitiesTab

/////////////////
//// @meshes ////
/////////////////
void MeshesTab(){
	persist bool rename_mesh = false;
	persist char rename_buffer[DESHI_NAME_SIZE] = {};
	persist u32  sel_mesh_idx = -1;
	persist int sel_vertex_idx = -1;
	persist int sel_triangle_idx = -1;
	persist int sel_face_idx = -1;
	Mesh* selected = nullptr;
	if(sel_mesh_idx < Storage::MeshCount()) selected = Storage::MeshAt(sel_mesh_idx);
	
	//// selected mesh keybinds ////
	//start renaming mesh
	if(selected && DeshInput->KeyPressed(Key::F2)){
		rename_mesh = true;
		DeshConsole->IMGUI_KEY_CAPTURE = true;
		cpystr(rename_buffer, selected->name, DESHI_NAME_SIZE);
	}
	//submit renaming mesh
	if(selected && rename_mesh && DeshInput->KeyPressed(Key::ENTER)){
		rename_mesh = false;
		DeshConsole->IMGUI_KEY_CAPTURE = false;
		cpystr(selected->name, rename_buffer, DESHI_NAME_SIZE);
	}
	//stop renaming mesh
	if(rename_mesh && DeshInput->KeyPressed(Key::ESCAPE)){
		rename_mesh = false;
		DeshConsole->IMGUI_KEY_CAPTURE = false;
	}
	//delete mesh
	if(selected && DeshInput->KeyPressed(Key::DELETE)){
		//Storage::DeleteMesh(sel_mesh_idx);
		//sel_mat_idx = -1;
	}
	
	//// mesh list panel ////
	SetPadding;
	if(ImGui::BeginChild("##mesh_list", ImVec2(ImGui::GetWindowWidth() * 0.95, ImGui::GetWindowHeight() * .14f), false)){
		if(ImGui::BeginTable("##mesh_table", 3, ImGuiTableFlags_BordersInner)){
			ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, fonth * 2.5f);
			ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, fonth);
			
			forX(mesh_idx, Storage::MeshCount()){
				ImGui::PushID(Storage::MeshAt(mesh_idx));
				ImGui::TableNextRow();
				
				//// id + label ////
				ImGui::TableSetColumnIndex(0);
				char label[8];
				sprintf(label, " %03d", mesh_idx);
				if(ImGui::Selectable(label, sel_mesh_idx == mesh_idx, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap)){
					sel_mesh_idx = (ImGui::GetIO().KeyCtrl) ? -1 : mesh_idx; //deselect if CTRL held
					rename_mesh = false;
					DeshConsole->IMGUI_KEY_CAPTURE = false;
					sel_vertex_idx = -1;
					sel_triangle_idx = -1;
					sel_face_idx = -1;
				}
				
				//// name text ////
				ImGui::TableSetColumnIndex(1);
				if(rename_mesh && sel_mesh_idx == mesh_idx){
					ImGui::PushStyleColor(ImGuiCol_FrameBg, ImColor(0xff203c56).Value);
					ImGui::InputText("##mesh_rename_input", rename_buffer, DESHI_NAME_SIZE, ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue);
					ImGui::PopStyleColor();
				}
				else{
					ImGui::TextEx(Storage::MeshName(mesh_idx));
				}
				
				//// delete button ////
				ImGui::TableSetColumnIndex(2);
				if(ImGui::Button("X", ImVec2(-FLT_MIN, 0.0f))){
					if(mesh_idx == sel_mesh_idx){
						sel_mesh_idx = -1;
					}
					else if(sel_mesh_idx != -1 && sel_mesh_idx > mesh_idx){
						sel_mesh_idx -= 1;
					}
					Storage::DeleteMesh(mesh_idx);
					sel_vertex_idx = -1;
					sel_triangle_idx = -1;
					sel_face_idx = -1;
				}
				ImGui::PopID();
			}
			ImGui::EndTable(); //mesh_table
		}
		ImGui::EndChild(); //mesh_list
	}
	
	ImGui::Separator();
	
	//// create new mesh button ////
	ImGui::SetCursorPosX(ImGui::GetWindowWidth() * 0.025); //half of 1 - 0.95
	if(ImGui::Button("Load New Mesh", ImVec2(ImGui::GetWindowWidth() * 0.95, 0.0f))){
		//!Incomplete
		ImGui::TextEx("TODO    Editor::FileSelector");
	}
	
	ImGui::Separator();
	if(selected == nullptr || sel_mesh_idx == -1) return;
	//// selected mesh inspector panel ////
	persist bool vertex_all = true;
	persist bool vertex_draw = true;
	persist bool vertex_indexes = false;
	persist bool vertex_normals = false;
	persist bool triangle_all = true;
	persist bool triangle_draw = true;
	persist bool triangle_indexes = false;
	persist bool triangle_centers = false;
	persist bool triangle_normals = false;
	persist bool triangle_neighbors = false;
	persist bool trinei_indexes = false;
	persist bool triedge_indexes = false;
	persist bool face_all = true;
	persist bool face_draw = false;
	persist bool face_indexes = false;
	persist bool facenei_indexes = false;
	persist bool face_centers = false;
	persist bool face_normals = false;
	persist bool face_vertexes = false;
	persist bool face_triangles = false;
	persist bool face_outer_vertexes = false;
	persist bool face_vertex_indexes = false;
	persist bool face_outvertex_indexes = false;
	persist bool face_triangle_indexes = false;
	persist bool face_facenei_indexes = false;
	persist bool face_trinei_indexes = false;
	persist bool face_face_neighbors = false;
	persist bool face_tri_neighbors = false;
	persist color text_color = Color_White;
	color vertex_color = Color_Green; //non-static so they can be changed
	color triangle_color = Color_Red;
	color face_color = Color_Blue;
	persist color selected_color = color(255, 255, 0, 128); //yellow  half-alpha
	persist color neighbor_color = color(255, 0, 255, 128); //megenta half-alpha
	persist color edge_color     = color(0, 255, 255, 128); //cyan    half-alpha
	persist vec3 off{ .005f,.005f,.005f }; //slight offset for possibly overlapping things //TODO(delle) add wide lines to vulkan
	persist f32 scale = 1.f;
	persist f32 normal_scale = .3f; //scale for making normal lines smaller
	persist f32 sel_vertex_colors[3];
	
	SetPadding;
	if(ImGui::BeginChild("##mesh_inspector", ImVec2(ImGui::GetWindowWidth() * .95f, ImGui::GetWindowHeight() * .8f), false)){
		//// name ////
		ImGui::TextEx("Name"); ImGui::SameLine(); ImGui::SetNextItemWidth(-FLT_MIN);
		ImGui::InputText("##mat_name_input", selected->name, DESHI_NAME_SIZE, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);
		
		ImGui::Separator();
		//// stats and scale ////
		ImGui::TextEx("Stats"); ImGui::SameLine();
		int planar_vertex_count = 0;
		forI(selected->faceCount){ planar_vertex_count += selected->faces[i].vertexCount; }
		ImGui::HelpMarker("(+)", TOSTRING("Vertex   Count: ", selected->vertexCount,
										  "\nIndex    Count: ", selected->indexCount,
										  "\nTriangle Count: ", selected->triangleCount,
										  "\nFace     Count: ", selected->faceCount,
										  "\nPlanar Vertex Count: ", planar_vertex_count).str);
		ImGui::TextEx("Draw Scale"); ImGui::SameLine(); ImGui::SetNextItemWidth(-1);
		ImGui::InputFloat("##mi_scale", &scale, 0, 0);
		ImGui::TextEx("Normal Scale"); ImGui::SameLine(); ImGui::SetNextItemWidth(-1);
		ImGui::InputFloat("##mi_scale_normal", &normal_scale, 0, 0);
		
		ImGui::Separator();
		//// select part ////
		ImGui::TextEx("Vertex   "); ImGui::SameLine(); ImGui::SetNextItemWidth(-1);
		if(ImGui::InputInt("##mi_vertex_input", &sel_vertex_idx, 1, 10)){
			sel_vertex_idx = Clamp(sel_vertex_idx, -1, selected->vertexCount - 1);
		}
		if(sel_vertex_idx != -1){
			Mesh::Vertex* sel_vertex = &selected->vertexes[sel_vertex_idx];
			ImGui::Text("Positon: (%.2f,%.2f,%.2f)", sel_vertex->pos.x, sel_vertex->pos.y, sel_vertex->pos.z);
			ImGui::Text("Normal : (%.2f,%.2f,%.2f)", sel_vertex->normal.x, sel_vertex->normal.y, sel_vertex->normal.z);
			ImGui::Text("UV     : (%.2f,%.2f)", sel_vertex->uv.u, sel_vertex->uv.v);
		}
		ImGui::TextEx("Triangle "); ImGui::SameLine(); ImGui::SetNextItemWidth(-1);
		if(ImGui::InputInt("##mi_tri_input", &sel_triangle_idx, 1, 10)){
			sel_triangle_idx = Clamp(sel_triangle_idx, -1, selected->triangleCount - 1);
		}
		if(sel_triangle_idx != -1){
			Mesh::Triangle* sel_triangle = &selected->triangles[sel_triangle_idx];
			vec3 tri_center = MeshTriangleMidpoint(sel_triangle);
			ImGui::Text("Vertex 0: (%.2f,%.2f,%.2f)", sel_triangle->p[0].x, sel_triangle->p[0].y, sel_triangle->p[0].z);
			ImGui::Text("Vertex 1: (%.2f,%.2f,%.2f)", sel_triangle->p[1].x, sel_triangle->p[1].y, sel_triangle->p[1].z);
			ImGui::Text("Vertex 2: (%.2f,%.2f,%.2f)", sel_triangle->p[2].x, sel_triangle->p[2].y, sel_triangle->p[2].z);
			ImGui::Text("Normal  : (%.2f,%.2f,%.2f)", sel_triangle->normal.x, sel_triangle->normal.y, sel_triangle->normal.z);
			ImGui::Text("Center  : (%.2f,%.2f,%.2f)", tri_center.x, tri_center.y, tri_center.z);
		}
		ImGui::TextEx("Face     "); ImGui::SameLine(); ImGui::SetNextItemWidth(-1);
		if(ImGui::InputInt("##mi_face_input", &sel_face_idx, 1, 10)){
			sel_face_idx = Clamp(sel_face_idx, -1, selected->faceCount - 1);
		}
		if(sel_face_idx != -1){
			Mesh::Face* sel_face = &selected->faces[sel_face_idx];
			ImGui::Text("Normal: (%.2f,%.2f,%.2f)", sel_face->normal.x, sel_face->normal.y, sel_face->normal.z);
			ImGui::Text("Center: (%.2f,%.2f,%.2f)", sel_face->center.x, sel_face->center.y, sel_face->center.z);
		}
		
		ImGui::Separator();
		//// inspector tabs ////
		if(ImGui::BeginTabBar("MeshInspectorTabs")){
			if(ImGui::BeginTabItem("Vertexes")){
				ImGui::Checkbox("Show All", &vertex_all);
				ImGui::Checkbox("Draw Unselected", &vertex_draw);
				ImGui::Checkbox("Indexes", &vertex_indexes);
				ImGui::Checkbox("Normals", &vertex_normals);
				ImGui::EndTabItem();
			}
			if(ImGui::BeginTabItem("Triangles")){
				ImGui::Checkbox("Show All", &triangle_all);
				ImGui::Checkbox("Draw Unselected", &triangle_draw);
				ImGui::Checkbox("Indexes", &triangle_indexes);
				ImGui::Checkbox("Centers", &triangle_centers);
				ImGui::Checkbox("Normals", &triangle_normals);
				ImGui::Checkbox("Neighbors", &triangle_neighbors);
				ImGui::Checkbox("Neighbor Indexes", &trinei_indexes);
				ImGui::Checkbox("Edge Indexes", &triedge_indexes);
				ImGui::EndTabItem();
			}
			if(ImGui::BeginTabItem("Faces")){
				ImGui::Checkbox("Show All", &face_all);
				ImGui::Checkbox("Draw Unselected", &face_draw);
				ImGui::Checkbox("Indexes", &face_indexes);
				ImGui::Checkbox("Centers", &face_centers);
				ImGui::Checkbox("Normals", &face_normals);
				ImGui::Checkbox("Vertexes", &face_vertexes);
				ImGui::SameLine(); ImGui::Checkbox("Indexes##fv", &face_vertex_indexes);
				ImGui::Checkbox("Outer Vertexes", &face_outer_vertexes);
				ImGui::SameLine(); ImGui::Checkbox("Indexes##fov", &face_outvertex_indexes);
				ImGui::Checkbox("Triangles", &face_triangles);
				ImGui::SameLine(); ImGui::Checkbox("Indexes##ft", &face_triangle_indexes);
				ImGui::Checkbox("Triangle Neighbors", &face_tri_neighbors);
				ImGui::SameLine(); ImGui::Checkbox("Indexes##ftn", &face_trinei_indexes);
				ImGui::Checkbox("Face Neighbors", &face_face_neighbors);
				ImGui::SameLine(); ImGui::Checkbox("Indexes##ffn", &face_facenei_indexes);
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}
		
		//// draw the stuffs //// //TODO(delle) compress these since they mostly duplicate except color
		ImGui::BeginDebugLayer();
		
		//// vertexes ////
		if(sel_vertex_idx != -1){
			Mesh::Vertex* sel_vertex = &selected->vertexes[sel_vertex_idx];
			Render::DrawBoxFilled(mat4::TransformationMatrix(sel_vertex->pos * scale, vec3::ZERO, vec3{ .05f,.05f,.05f }), selected_color);
			if(vertex_indexes) ImGui::DebugDrawText3(TOSTRING("V", sel_vertex_idx).str, sel_vertex->pos * scale, text_color, vec2{ -5,-5 });
			if(vertex_normals) Render::DrawLine(sel_vertex->pos * scale, sel_vertex->pos * scale + sel_vertex->normal * normal_scale, selected_color);
		}
		if(vertex_all){
			forI(selected->vertexCount){
				if(i == sel_vertex_idx) continue;
				Mesh::Vertex* sel_vertex = &selected->vertexes[i];
				if(vertex_draw) Render::DrawBoxFilled(mat4::TransformationMatrix(sel_vertex->pos * scale, vec3::ZERO, vec3{ .03f,.03f,.03f }), vertex_color);
				if(vertex_indexes) ImGui::DebugDrawText3(TOSTRING("V", i).str, sel_vertex->pos * scale, text_color, vec2{ -5,-5 });
				if(vertex_normals) Render::DrawLine(sel_vertex->pos * scale, sel_vertex->pos * scale + sel_vertex->normal * normal_scale, vertex_color);
			}
		}
		
		//// triangles ////
		if(sel_triangle_idx != -1){
			Mesh::Triangle* sel_triangle = &selected->triangles[sel_triangle_idx];
			vec3 tri_center = MeshTriangleMidpoint(sel_triangle) * scale;
			Render::DrawTriangleFilled(sel_triangle->p[0] * scale, sel_triangle->p[1] * scale, sel_triangle->p[2] * scale, selected_color);
			if(triangle_indexes) ImGui::DebugDrawText3(TOSTRING("T", sel_triangle_idx).str, tri_center, text_color, vec2{ -5,-5 });
			if(triangle_centers) Render::DrawBoxFilled(mat4::TransformationMatrix(tri_center, vec3::ZERO, vec3{ .05f,.05f,.05f }), selected_color);
			if(triangle_normals) Render::DrawLine(tri_center, tri_center + sel_triangle->normal * normal_scale, selected_color);
			forX(tni, sel_triangle->neighbors.count){
				Mesh::Triangle* tri_nei = &selected->triangles[sel_triangle->neighbors[tni]];
				if(trinei_indexes) ImGui::DebugDrawText3(TOSTRING("TN", tni).str, MeshTriangleMidpoint(tri_nei) * scale, text_color, vec2{ 10,10 });
				if(triangle_neighbors) Render::DrawTriangleFilled(tri_nei->p[0] * scale, tri_nei->p[1] * scale, tri_nei->p[2] * scale, neighbor_color);
				int e0 = (sel_triangle->edges[tni] == 0) ? 0 : (sel_triangle->edges[tni] == 1) ? 1 : 2;
				int e1 = (sel_triangle->edges[tni] == 0) ? 1 : (sel_triangle->edges[tni] == 1) ? 2 : 0;
				if(triedge_indexes) ImGui::DebugDrawText3(TOSTRING("TE", sel_triangle->edges[tni]).str, Math::LineMidpoint(sel_triangle->p[e0], sel_triangle->p[e1]) * scale, text_color, vec2{ -5,-5 });
			}
		}
		if(triangle_all){
			forI(selected->triangleCount){
				if(i == sel_triangle_idx) continue;
				Mesh::Triangle* sel_triangle = &selected->triangles[i];
				vec3 tri_center = MeshTriangleMidpoint(sel_triangle) * scale;
				if(triangle_draw) Render::DrawTriangle(sel_triangle->p[0] * scale, sel_triangle->p[1] * scale, sel_triangle->p[2] * scale, triangle_color);
				if(triangle_indexes) ImGui::DebugDrawText3(TOSTRING("T", i).str, tri_center, text_color, vec2{ -5,-5 });
				if(triangle_centers) Render::DrawBoxFilled(mat4::TransformationMatrix(tri_center, vec3::ZERO, vec3{ .03f,.03f,.03f }), triangle_color);
				if(triangle_normals) Render::DrawLine(tri_center, tri_center + sel_triangle->normal * normal_scale, triangle_color);
			}
		}
		
		//// faces ////
		if(sel_face_idx != -1){
			Mesh::Face* sel_face = &selected->faces[sel_face_idx];
			if(face_indexes) ImGui::DebugDrawText3(TOSTRING("F", sel_face_idx).str, sel_face->center * scale, text_color, vec2{ -5,-5 });
			if(face_centers) Render::DrawBoxFilled(mat4::TransformationMatrix(sel_face->center * scale, vec3::ZERO, vec3{ .05f,.05f,.05f }), selected_color);
			if(face_normals) Render::DrawLine(sel_face->center * scale, sel_face->center * scale + sel_face->normal * normal_scale, selected_color);
			forX(fvi, sel_face->vertexCount){
				MeshVertex* fv = &selected->vertexes[sel_face->vertexes[fvi]];
				if(face_vertexes) Render::DrawBoxFilled(mat4::TransformationMatrix(fv->pos * scale, vec3::ZERO, vec3{ .05f,.05f,.05f }), edge_color);
				if(face_vertex_indexes) ImGui::DebugDrawText3(TOSTRING("FV", fvi).str, fv->pos * scale, text_color, vec2{ -5,-5 });
			}
			forX(fvi, sel_face->outerVertexCount){
				MeshVertex* fv = &selected->vertexes[sel_face->outerVertexes[fvi]];
				if(face_outer_vertexes) Render::DrawBoxFilled(mat4::TransformationMatrix(fv->pos * scale, vec3::ZERO, vec3{ .05f,.05f,.05f }), edge_color);
				if(face_outvertex_indexes) ImGui::DebugDrawText3(TOSTRING("FOV", fvi).str, fv->pos * scale, text_color, vec2{ 10,10 });
			}
			forX(fti, sel_face->triangleCount){
				MeshTriangle* ft = &selected->triangles[sel_face->triangles[fti]];
				Render::DrawTriangleFilled(ft->p[0] * scale, ft->p[1] * scale, ft->p[2] * scale, selected_color);
				if(face_triangles) Render::DrawTriangle(ft->p[0] * scale, ft->p[1] * scale, ft->p[2] * scale, text_color);
				if(face_triangle_indexes) ImGui::DebugDrawText3(TOSTRING("FT", fti).str, MeshTriangleMidpoint(ft) * scale, text_color, vec2{ -10,-10 });
			}
			forX(fnti, sel_face->neighborTriangleCount){
				MeshTriangle* ft = &selected->triangles[sel_face->triangleNeighbors[fnti]];
				if(face_tri_neighbors) Render::DrawTriangleFilled(ft->p[0] * scale, ft->p[1] * scale, ft->p[2] * scale, neighbor_color);
				if(face_trinei_indexes) ImGui::DebugDrawText3(TOSTRING("FTN", fnti).str, MeshTriangleMidpoint(ft) * scale, text_color, vec2{ 10,10 });
			}
			forX(fnfi, sel_face->neighborFaceCount){
				MeshFace* ff = &selected->faces[sel_face->faceNeighbors[fnfi]];
				if(face_face_neighbors){
					forX(ffti, ff->triangleCount){
						MeshTriangle* fft = &selected->triangles[ff->triangles[ffti]];
						Render::DrawTriangleFilled(fft->p[0] * scale, fft->p[1] * scale, fft->p[2] * scale, edge_color);
					}
				}
				if(face_facenei_indexes) ImGui::DebugDrawText3(TOSTRING("FFN", fnfi).str, ff->center * scale, text_color, vec2{ 10,10 });
			}
		}
		if(face_all){
			forX(fi, selected->faceCount){
				if(fi == sel_face_idx) continue;
				Mesh::Face* sel_face = &selected->faces[fi];
				//array<u32>  f_vertexes(sel_face->outerVertexArray, sel_face->outerVertexCount); f_vertexes.BubbleSort();
				//array<vec3> f_points(sel_face->outerVertexCount);
				//forX(fovi, sel_face->outerVertexCount) f_points.add(selected->vertexes[f_vertexes[fovi]].pos+off);
				//Render::DrawPoly(f_points, face_color); //TODO(delle) fix drawing outline
				if(face_draw){
					forX(fti, sel_face->triangleCount){
						MeshTriangle* ft = &selected->triangles[sel_face->triangles[fti]];
						Render::DrawTriangle(ft->p[0] * scale - off, ft->p[1] * scale - off, ft->p[2] * scale - off, face_color);
					}
				}
				if(face_indexes) ImGui::DebugDrawText3(TOSTRING("F", fi).str, sel_face->center * scale, text_color, vec2{ -5,-5 });
				if(face_centers) Render::DrawBoxFilled(mat4::TransformationMatrix(sel_face->center * scale, vec3::ZERO, vec3{ .05f,.05f,.05f }), face_color);
				if(face_normals) Render::DrawLine(sel_face->center * scale, sel_face->center * scale + sel_face->normal * normal_scale, face_color);
			}
		}
		
		ImGui::EndDebugLayer();
		
		ImGui::EndChild(); //mesh_inspector
	}
} //MeshesTab

///////////////////
//// @textures ////
///////////////////
void TexturesTab(){
	persist u32 sel_tex_idx = -1;
	Texture* selected = nullptr;
	if(sel_tex_idx < Storage::TextureCount()) selected = Storage::TextureAt(sel_tex_idx);
	
	//// selected material keybinds ////
	//delete material
	if(selected && DeshInput->KeyPressed(Key::DELETE)){
		//TODO(Ui) re-enable this with a popup to delete OR with undoing on delete
		//Storage::DeleteTexture(sel_tex_idx);
		//sel_tex_idx = -1;
	}
	
	//// material list panel ////
	SetPadding;
	if(ImGui::BeginChild("##tex_list", ImVec2(ImGui::GetWindowWidth() * 0.95, ImGui::GetWindowHeight() * .14f), false)){
		if(ImGui::BeginTable("##tex_table", 3, ImGuiTableFlags_BordersInner)){
			ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, fonth * 2.5f);
			ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, fonth);
			
			forX(tex_idx, Storage::TextureCount()){
				ImGui::PushID(tex_idx);
				ImGui::TableNextRow();
				
				//// id + label ////
				ImGui::TableSetColumnIndex(0);
				char label[8];
				sprintf(label, " %03d", tex_idx);
				if(ImGui::Selectable(label, sel_tex_idx == tex_idx, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap)){
					sel_tex_idx = (ImGui::GetIO().KeyCtrl) ? -1 : tex_idx; //deselect if CTRL held
				}
				
				//// name text ////
				ImGui::TableSetColumnIndex(1);
				ImGui::TextEx(Storage::TextureName(tex_idx));
				
				//// delete button ////
				ImGui::TableSetColumnIndex(2);
				if(ImGui::Button("X", ImVec2(-FLT_MIN, 0.0f))){
					if(tex_idx == sel_tex_idx){
						sel_tex_idx = -1;
					}
					else if(sel_tex_idx != -1 && sel_tex_idx > tex_idx){
						sel_tex_idx -= 1;
					}
					Storage::DeleteTexture(tex_idx);
				}
				ImGui::PopID();
			}
			ImGui::EndTable(); //tex_table
		}
		ImGui::EndChild(); //tex_list
	}
	
	ImGui::Separator();
	
	//// create new texture button ////
	ImGui::SetCursorPosX(ImGui::GetWindowWidth() * 0.025); //half of 1 - 0.95
	if(ImGui::Button("Load New Texture", ImVec2(ImGui::GetWindowWidth() * 0.95, 0.0f))){
		//!Incomplete
		ImGui::TextEx("TODO    Editor::FileSelector");
	}
	
	ImGui::Separator();
	
	//// selected material inspector panel ////
	if(selected == nullptr) return;
	SetPadding;
	if(ImGui::BeginChild("##tex_inspector", ImVec2(ImGui::GetWindowWidth() * .95f, ImGui::GetWindowHeight() * .8f), false)){
		//// image preview ////
		ImGui::TextCentered("Image Preview");
		//!Incomplete
		ImGui::TextEx("TODO    Render::DrawImage");
		
		ImGui::EndChild(); //tex_inspector
	}
} //TexturesTab

////////////////////
//// @materials ////
////////////////////
void MaterialsTab(){
	persist u32  sel_mat_idx = -1;
	persist bool rename_mat = false;
	persist char rename_buffer[DESHI_NAME_SIZE] = {};
	Material* selected = nullptr;
	if(sel_mat_idx < Storage::MaterialCount()) selected = Storage::MaterialAt(sel_mat_idx);
	
	//// selected material keybinds ////
	//start renaming material
	if(selected && DeshInput->KeyPressed(Key::F2)){
		rename_mat = true;
		DeshConsole->IMGUI_KEY_CAPTURE = true;
		cpystr(rename_buffer, selected->name, DESHI_NAME_SIZE);
	}
	//submit renaming material
	if(selected && rename_mat && DeshInput->KeyPressed(Key::ENTER)){
		rename_mat = false;
		DeshConsole->IMGUI_KEY_CAPTURE = false;
		cpystr(selected->name, rename_buffer, DESHI_NAME_SIZE);
	}
	//stop renaming material
	if(rename_mat && DeshInput->KeyPressed(Key::ESCAPE)){
		rename_mat = false;
		DeshConsole->IMGUI_KEY_CAPTURE = false;
	}
	//delete material
	if(selected && DeshInput->KeyPressed(Key::DELETE)){
		//TODO(Ui) re-enable this with a popup to delete OR with undoing on delete
		//Storage::DeleteMaterial(sel_mat_idx);
		//sel_mat_idx = -1;
	}
	
	//// material list panel ////
	SetPadding;
	if(ImGui::BeginChild("##mat_list", ImVec2(ImGui::GetWindowWidth() * 0.95, ImGui::GetWindowHeight() * .14f), false)){
		if(ImGui::BeginTable("##mat_table", 3, ImGuiTableFlags_BordersInner)){
			ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, fonth * 2.5f);
			ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, fonth);
			
			forX(mat_idx, Storage::MaterialCount()){
				ImGui::PushID(mat_idx);
				ImGui::TableNextRow();
				
				//// id + label ////
				ImGui::TableSetColumnIndex(0);
				char label[8];
				sprintf(label, " %03d", mat_idx);
				if(ImGui::Selectable(label, sel_mat_idx == mat_idx, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap)){
					sel_mat_idx = (ImGui::GetIO().KeyCtrl) ? -1 : mat_idx; //deselect if CTRL held
					rename_mat = false;
					DeshConsole->IMGUI_KEY_CAPTURE = false;
				}
				
				//// name text ////
				ImGui::TableSetColumnIndex(1);
				if(rename_mat && sel_mat_idx == mat_idx){
					ImGui::PushStyleColor(ImGuiCol_FrameBg, ImColor(0xff203c56).Value);
					ImGui::InputText("##mat_rename_input", rename_buffer, DESHI_NAME_SIZE, ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue);
					ImGui::PopStyleColor();
				}
				else{
					ImGui::TextEx(Storage::MaterialName(mat_idx));
				}
				
				//// delete button ////
				ImGui::TableSetColumnIndex(2);
				if(ImGui::Button("X", ImVec2(-FLT_MIN, 0.0f))){
					if(mat_idx == sel_mat_idx){
						sel_mat_idx = -1;
					}
					else if(sel_mat_idx != -1 && sel_mat_idx > mat_idx){
						sel_mat_idx -= 1;
					}
					Storage::DeleteMaterial(mat_idx);
				}
				ImGui::PopID();
			}
			ImGui::EndTable(); //mat_table
		}
		ImGui::EndChild(); //mat_list
	}
	
	ImGui::Separator();
	
	//// create new material button ////
	ImGui::SetCursorPosX(ImGui::GetWindowWidth() * 0.025); //half of 1 - 0.95
	if(ImGui::Button("Create New Material", ImVec2(ImGui::GetWindowWidth() * 0.95, 0.0f))){
		auto new_mat = Storage::CreateMaterial(TOSTRING("material", Storage::MaterialCount()).str, Shader_PBR);
		sel_mat_idx = new_mat.first;
		selected = new_mat.second;
	}
	
	ImGui::Separator();
	
	//// selected material inspector panel ////
	if(selected == nullptr) return;
	SetPadding;
	if(ImGui::BeginChild("##mat_inspector", ImVec2(ImGui::GetWindowWidth() * .95f, ImGui::GetWindowHeight() * .8f), false)){
		//// name ////
		ImGui::TextEx("Name   "); ImGui::SameLine(); ImGui::SetNextItemWidth(-FLT_MIN);
		ImGui::InputText("##mat_name_input", selected->name, DESHI_NAME_SIZE, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);
		
		//// shader selection ////
		ImGui::TextEx("Shader "); ImGui::SameLine(); ImGui::SetNextItemWidth(-1);
		if(ImGui::BeginCombo("##mat_shader_combo", ShaderStrings[selected->shader])){
			forI(ArrayCount(ShaderStrings)){
				if(ImGui::Selectable(ShaderStrings[i], selected->shader == i)){
					selected->shader = i;
					Render::UpdateMaterial(selected);
				}
			}
			ImGui::EndCombo(); //mat_shader_combo
		}
		
		ImGui::Separator();
		
		//// material properties ////
		//TODO(delle) setup material editing other than PBR once we have material parameters
		switch (selected->shader){
			//// flat shader ////
			case Shader_Flat: {
				
			}break;
			
			//// PBR shader ////
			//TODO(Ui) add texture image previews
			case Shader_PBR:default: {
				forX(mti, selected->textures.count){
					ImGui::TextEx(TOSTRING("Texture ", mti).str); ImGui::SameLine(); ImGui::SetNextItemWidth(-1);
					if(ImGui::BeginCombo(TOSTRING("##mat_texture_combo", mti).str, Storage::TextureName(selected->textures[mti]))){
						dir_textures = Assets::iterateDirectory(Assets::dirTextures());
						forX(ti, dir_textures.count){
							if(ImGui::Selectable(dir_textures[ti].str, strcmp(Storage::TextureName(selected->textures[mti]), dir_textures[ti].str) == 0)){
								selected->textures[mti] = Storage::CreateTextureFromFile(dir_textures[ti].str).first;
								Render::UpdateMaterial(selected);
							}
						}
						ImGui::EndCombo();
					}
				}
			}break;
		}
		
		if(ImGui::Button("Add Texture", ImVec2(-1, 0))){
			selected->textures.add(0);
			Render::UpdateMaterial(selected);
		}
		
		ImGui::EndChild(); //mat_inspector
	}
} //MaterialsTab

/////////////////
//// @models ////
/////////////////
void ModelsTab(){
	persist u32  sel_model_idx = -1;
	persist u32  sel_batch_idx = -1;
	persist bool rename_model = false;
	persist char rename_buffer[DESHI_NAME_SIZE] = {};
	Model* selected = nullptr;
	if(sel_model_idx < Storage::ModelCount()) selected = Storage::ModelAt(sel_model_idx);
	
	//// selected model keybinds ////
	//start renaming model
	if(selected && DeshInput->KeyPressed(Key::F2)){
		rename_model = true;
		DeshConsole->IMGUI_KEY_CAPTURE = true;
		cpystr(rename_buffer, selected->name, DESHI_NAME_SIZE);
	}
	//submit renaming model
	if(selected && rename_model && DeshInput->KeyPressed(Key::ENTER)){
		rename_model = false;
		DeshConsole->IMGUI_KEY_CAPTURE = false;
		cpystr(selected->name, rename_buffer, DESHI_NAME_SIZE);
	}
	//stop renaming model
	if(rename_model && DeshInput->KeyPressed(Key::ESCAPE)){
		rename_model = false;
		DeshConsole->IMGUI_KEY_CAPTURE = false;
	}
	//delete model
	if(selected && DeshInput->KeyPressed(Key::DELETE)){
		//TODO(Ui) re-enable this with a popup to delete OR with undoing on delete
		//Storage::DeleteModel(sel_model_idx);
		//sel_model_idx = -1;
	}
	
	//// model list panel ////
	SetPadding;
	if(ImGui::BeginChild("##model_list", ImVec2(ImGui::GetWindowWidth() * 0.95, ImGui::GetWindowHeight() * .14f), false)){
		if(ImGui::BeginTable("##model_table", 3, ImGuiTableFlags_BordersInner)){
			ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, fonth * 2.5f);
			ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, fonth);
			
			forX(model_idx, Storage::ModelCount()){
				ImGui::PushID(Storage::ModelAt(model_idx));
				ImGui::TableNextRow();
				
				//// id + label ////
				ImGui::TableSetColumnIndex(0);
				char label[8];
				sprintf(label, " %03d", model_idx);
				if(ImGui::Selectable(label, sel_model_idx == model_idx, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap)){
					sel_model_idx = (ImGui::GetIO().KeyCtrl) ? -1 : model_idx; //deselect if CTRL held
					rename_model = false;
					DeshConsole->IMGUI_KEY_CAPTURE = false;
				}
				
				//// name text ////
				ImGui::TableSetColumnIndex(1);
				if(rename_model && sel_model_idx == model_idx){
					ImGui::PushStyleColor(ImGuiCol_FrameBg, ImColor(0xff203c56).Value);
					ImGui::InputText("##model_rename_input", rename_buffer, DESHI_NAME_SIZE, ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue);
					ImGui::PopStyleColor();
				}
				else{
					ImGui::TextEx(Storage::ModelName(model_idx));
				}
				
				//// delete button ////
				ImGui::TableSetColumnIndex(2);
				//!BUG -FLT_MIN aligns this button improperly
				if(ImGui::Button("X", ImVec2(-FLT_MIN, 0.0f))){
					if(model_idx == sel_model_idx){
						sel_model_idx = -1;
					}
					else if(sel_model_idx != -1 && sel_model_idx > model_idx){
						sel_model_idx -= 1;
					}
					Storage::DeleteModel(model_idx);
				}
				ImGui::PopID();
			}
			ImGui::EndTable(); //model_table
		}
		ImGui::EndChild(); //model_list
	}
	
	ImGui::Separator();
	
	//// create new model button ////
	ImGui::SetCursorPosX(ImGui::GetWindowWidth() * 0.025); //half of 1 - 0.95
	if(ImGui::Button("Create New Model", ImVec2(ImGui::GetWindowWidth() * 0.95, 0.0f))){
		auto new_model = Storage::CopyModel(Storage::NullModel());
		sel_model_idx = new_model.first;
		selected = new_model.second;
		sel_batch_idx = -1;
	}
	
	ImGui::Separator();
	if(selected == nullptr) return;
	
	//// selected model inspector panel ////
	SetPadding;
	if(ImGui::BeginChild("##model_inspector", ImVec2(ImGui::GetWindowWidth() * .95f, ImGui::GetWindowHeight() * .8f), false)){
		//// name ////
		ImGui::TextEx("Name  "); ImGui::SameLine(); ImGui::SetNextItemWidth(-FLT_MIN);
		ImGui::InputText("##model_name_input", selected->name, DESHI_NAME_SIZE, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);
		
		//// mesh selection ////
		ImGui::TextEx("Mesh  "); ImGui::SameLine(); ImGui::SetNextItemWidth(-1);
		if(ImGui::BeginCombo("##model_mesh_combo", selected->mesh->name)){
			forI(Storage::MeshCount()){
				if(ImGui::Selectable(Storage::MeshName(i), selected->mesh == Storage::MeshAt(i))){
					selected->mesh = Storage::MeshAt(i);
					forX(batch_idx, selected->batches.count){
						selected->batches[batch_idx].indexOffset = 0;
						selected->batches[batch_idx].indexCount = selected->mesh->indexCount;
					}
				}
			}
			ImGui::EndCombo(); //model_mesh_combo
		}
		
		ImGui::Separator();
		
		//// batch selection ////
		ImGui::TextCentered("Batches");
		if(ImGui::BeginTable("##batch_table", 3, ImGuiTableFlags_None, ImVec2(-FLT_MIN, ImGui::GetWindowHeight() * .10f))){
			ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, fonth * 2.5f);
			ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, fonth);
			
			forX(batch_idx, selected->batches.count){
				ImGui::PushID(&selected->batches[batch_idx]);
				ImGui::TableNextRow();
				
				//// id + label ////
				ImGui::TableSetColumnIndex(0);
				char label[8];
				sprintf(label, " %02d", batch_idx);
				if(ImGui::Selectable(label, sel_batch_idx == batch_idx, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap)){
					sel_batch_idx = (ImGui::GetIO().KeyCtrl) ? -1 : batch_idx; //deselect if CTRL held
					rename_model = false;
					DeshConsole->IMGUI_KEY_CAPTURE = false;
				}
				
				//// name text ////
				ImGui::TableSetColumnIndex(1);
				ImGui::Text("Batch %d", batch_idx);
				
				//// delete button ////
				ImGui::TableSetColumnIndex(2); //NOTE there must be at least 1 batch on a model
				if(ImGui::Button("X", ImVec2(-FLT_MIN, 0.0f)) && selected->batches.count > 1){
					if(batch_idx == sel_batch_idx){
						sel_batch_idx = -1;
					}
					else if(sel_batch_idx != -1 && sel_batch_idx > batch_idx){
						sel_batch_idx -= 1;
					}
					selected->batches.remove(batch_idx);
				}
				ImGui::PopID();
			}
			ImGui::EndTable(); //batch_table
		}
		if(ImGui::Button("Add Batch", ImVec2(-1, 0))){
			selected->batches.add(Model::Batch{});
		}
		
		ImGui::Separator();
		//// batch properties ////
		if(sel_batch_idx != -1){
			persist bool highlight_batch_triangles = false;
			ImGui::Checkbox("Highlight Triangles", &highlight_batch_triangles);
			if(highlight_batch_triangles){
				//!Incomplete
				ImGui::TextEx("TODO    Render::FillTriangle");
			}
			
			ImGui::TextEx("Index Offset "); ImGui::SameLine(); ImGui::SetNextItemWidth(-1);
			if(ImGui::InputInt("##batch_index_offset_input", (int*)&selected->batches[sel_batch_idx].indexOffset, 0, 0)){
				selected->batches[sel_batch_idx].indexOffset =
					Clamp(selected->batches[sel_batch_idx].indexOffset, 0, selected->mesh->indexCount - 1);
			}
			
			ImGui::TextEx("Index Count  "); ImGui::SameLine(); ImGui::SetNextItemWidth(-1);
			if(ImGui::InputInt("##batch_index_count_input", (int*)&selected->batches[sel_batch_idx].indexCount, 0, 0)){
				selected->batches[sel_batch_idx].indexCount =
					Clamp(selected->batches[sel_batch_idx].indexCount, 0, selected->mesh->indexCount);
			}
			
			ImGui::TextEx("Material     "); ImGui::SameLine(); ImGui::SetNextItemWidth(-1);
			if(ImGui::BeginCombo("##batch_mat_combo", Storage::MaterialName(selected->batches[sel_batch_idx].material))){
				forI(Storage::MaterialCount()){
					if(ImGui::Selectable(Storage::MaterialName(i), selected->batches[sel_batch_idx].material == i)){
						selected->batches[sel_batch_idx].material = i;
					}
				}
				ImGui::EndCombo(); //batch_mat_combo
			}
		}
		
		ImGui::EndChild(); //model_inspector
	}
} //ModelsTab


////////////////
//// @fonts ////
////////////////
void FontsTab(){
	persist u32 sel_font_idx = -1;
	Font* selected = nullptr;
	//!Incomplete
	ImGui::TextEx("font previews not implemented yet");
} //FontsTab


///////////////////
//// @settings ////
///////////////////
void SettingsTab(){
	SetPadding;
	if(ImGui::BeginChild("##settings_tab", ImVec2(ImGui::GetWindowWidth() * 0.95f, ImGui::GetWindowHeight() * .9f))){
		//// physics properties ////
		if(ImGui::CollapsingHeader("Physics", 0)){
			ImGui::TextEx("Sim in Editor   "); ImGui::SameLine();
			if(BoolButton(AtmoAdmin->simulateInEditor, "editor_sim")){ ToggleBool(AtmoAdmin->simulateInEditor); }
			ImGui::TextEx("Physics Paused  "); ImGui::SameLine();
			if(BoolButton(AtmoAdmin->physics.paused, "pengine_pause")){ ToggleBool(AtmoAdmin->physics.paused); }
			ImGui::TextEx("Integrating     "); ImGui::SameLine();
			if(BoolButton(AtmoAdmin->physics.integrating, "pengine_integrate")){ ToggleBool(AtmoAdmin->physics.integrating); }
			ImGui::TextEx("Manifold Solving"); ImGui::SameLine();
			if(BoolButton(AtmoAdmin->physics.solving, "pengine_solve")){ ToggleBool(AtmoAdmin->physics.solving); }
			if(ImGui::Button("Step Forward", ImVec2(-FLT_MIN, 0))){ AtmoAdmin->physics.step = true; }
			ImGui::TextEx("Gravity             "); ImGui::SameLine(); ImGui::InputFloat("##pengine_gravity", &AtmoAdmin->physics.gravity);
			ImGui::TextEx("Min Linear Velocity "); ImGui::SameLine(); ImGui::InputFloat("##pengine_minvel", &AtmoAdmin->physics.minVelocity);
			ImGui::TextEx("Max Linear Velocity "); ImGui::SameLine(); ImGui::InputFloat("##pengine_maxvel", &AtmoAdmin->physics.maxVelocity);
			ImGui::TextEx("Min Angular Velocity"); ImGui::SameLine(); ImGui::InputFloat("##pengine_minrvel", &AtmoAdmin->physics.maxRotVelocity);
			ImGui::TextEx("Max Angular Velocity"); ImGui::SameLine(); ImGui::InputFloat("##pengine_maxrvel", &AtmoAdmin->physics.maxRotVelocity);
			
			ImGui::Separator();
		}
		
		//// camera properties ////
		if(ImGui::CollapsingHeader("Camera", 0)){
			if(ImGui::Button("Zero", ImVec2(ImGui::GetWindowWidth() * .45f, 0))){
				AtmoAdmin->camera.position = vec3::ZERO; AtmoAdmin->camera.rotation = vec3::ZERO;
			} ImGui::SameLine();
			if(ImGui::Button("Reset", ImVec2(ImGui::GetWindowWidth() * .45f, 0))){
				AtmoAdmin->camera.position = { 4.f,3.f,-4.f }; AtmoAdmin->camera.rotation = { 28.f,-45.f,0.f };
			}
			
			ImGui::TextEx("Position  "); ImGui::SameLine(); ImGui::Inputvec3("##cam_pos", &AtmoAdmin->camera.position);
			ImGui::TextEx("Rotation  "); ImGui::SameLine(); ImGui::Inputvec3("##cam_rot", &AtmoAdmin->camera.rotation);
			ImGui::TextEx("Near Clip "); ImGui::SameLine();
			if(ImGui::InputFloat("##cam_nearz", &AtmoAdmin->camera.nearZ)){
				AtmoAdmin->camera.UpdateProjectionMatrix();
			}
			ImGui::TextEx("Far Clip  "); ImGui::SameLine();
			if(ImGui::InputFloat("##cam_farz", &AtmoAdmin->camera.farZ)){
				AtmoAdmin->camera.UpdateProjectionMatrix();
			};
			ImGui::TextEx("FOV       "); ImGui::SameLine();
			if(ImGui::InputFloat("##cam_fov", &AtmoAdmin->camera.fov)){
				AtmoAdmin->camera.UpdateProjectionMatrix();
			};
			
			ImGui::Separator();
		}
		
		//// render settings ////
		if(ImGui::CollapsingHeader("Rendering", 0)){
			local RenderSettings* settings = Render::GetSettings();
			local const char* resolution_strings[] = { "128", "256", "512", "1024", "2048", "4096" };
			local u32 resolution_values[] = { 128, 256, 512, 1024, 2048, 4096 };
			local u32 shadow_resolution_index = 4;
			local const char* msaa_strings[] = { "1", "2", "4", "8", "16", "32", "64" };
			local u32 msaa_index = settings->msaaSamples;
			local vec3 clear_color = settings->clearColor;
			local vec4 selected_color = settings->selectedColor;
			local vec4 collider_color = settings->colliderColor;
			
			ImGui::Checkbox("Debugging", (bool*)&settings->debugging);
			ImGui::Checkbox("Shader printf", (bool*)&settings->printf);
			ImGui::Checkbox("Recompile all shaders", (bool*)&settings->recompileAllShaders);
			ImGui::TextEx("MSAA Samples"); ImGui::SameLine(); ImGui::SetNextItemWidth(-1);
			if(ImGui::BeginCombo("##rs_msaa_combo", msaa_strings[msaa_index])){
				forI(ArrayCount(msaa_strings)){
					if(ImGui::Selectable(msaa_strings[i], msaa_index == i)){
						settings->msaaSamples = i;
						msaa_index = i;
					}
				}
				ImGui::EndCombo(); //rs_msaa_combo
			}
			ImGui::Checkbox("Texture Filtering", (bool*)&settings->textureFiltering);
			ImGui::Checkbox("Anistropic Filtering", (bool*)&settings->anistropicFiltering);
			ImGui::TextCentered("^ above settings require restart ^");
			ImGui::TextEx("Logging level"); ImGui::SameLine(); ImGui::SetNextItemWidth(-1);
			ImGui::SliderUInt32("##rs_logging_level", &settings->loggingLevel, 0, 4);
			ImGui::Checkbox("Crash on error", (bool*)&settings->crashOnError);
			ImGui::Checkbox("Compile shaders with optimization", (bool*)&settings->optimizeShaders);
			ImGui::Checkbox("Shadow PCF", (bool*)&settings->shadowPCF);
			ImGui::TextEx("Shadowmap resolution"); ImGui::SameLine(); ImGui::SetNextItemWidth(-1);
			if(ImGui::BeginCombo("##rs_shadowres_combo", resolution_strings[shadow_resolution_index])){
				forI(ArrayCount(resolution_strings)){
					if(ImGui::Selectable(resolution_strings[i], shadow_resolution_index == i)){
						settings->shadowResolution = resolution_values[i];
						shadow_resolution_index = i;
						Render::remakeOffscreen();
					}
				}
				ImGui::EndCombo(); //rs_shadowres_combo
			}
			ImGui::TextEx("Shadow near clip"); ImGui::SameLine(); ImGui::InputFloat("##rs_shadow_nearz", &settings->shadowNearZ);
			ImGui::TextEx("Shadow far clip"); ImGui::SameLine(); ImGui::InputFloat("##rs_shadow_farz", &settings->shadowFarZ);
			ImGui::TextEx("Shadow depth bias constant"); ImGui::SameLine(); ImGui::InputFloat("##rs_shadow_depthconstant", &settings->depthBiasConstant);
			ImGui::TextEx("Shadow depth bias slope"); ImGui::SameLine(); ImGui::InputFloat("##rs_shadow_depthslope", &settings->depthBiasSlope);
			ImGui::Checkbox("Show shadowmap texture", (bool*)&settings->showShadowMap);
			ImGui::TextEx("Background"); ImGui::SameLine();
			if(ImGui::ColorEdit3("##rs_clear_color", (f32*)&clear_color.r)){
				settings->clearColor = vec4(clear_color, 1.0f);
			}
			ImGui::TextEx("Selected  "); ImGui::SameLine();
			if(ImGui::ColorEdit4("##rs_selected_color", (f32*)&selected_color.r)){
				settings->selectedColor = selected_color;
			}
			ImGui::TextEx("Collider  "); ImGui::SameLine();
			if(ImGui::ColorEdit4("##rs_collider_color", (f32*)&collider_color.r)){
				settings->colliderColor = collider_color;
			}
			ImGui::Checkbox("Only show wireframe", (bool*)&settings->wireframeOnly);
			ImGui::Checkbox("Draw mesh wireframes", (bool*)&settings->meshWireframes);
			ImGui::Checkbox("Draw mesh normals", (bool*)&settings->meshNormals);
			ImGui::Checkbox("Draw light frustrums", (bool*)&settings->lightFrustrums);
			ImGui::Checkbox("Draw temp meshes on top", (bool*)&settings->tempMeshOnTop);
			if(ImGui::Button("Clear Debug Meshes")) Render::ClearDebug();
			
			ImGui::Separator();
		}
		
		ImGui::EndChild();
	}
}//settings tab


////////////////////
//// @inspector ////
////////////////////
void Inspector(){
	//window styling
	ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 5);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
	ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarRounding, 0);
	ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(0, 2));
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 0));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowTitleAlign, ImVec2(1, 0));
	ImGui::PushStyleVar(ImGuiStyleVar_TabRounding, 0);
	
	ImGui::PushStyleColor(ImGuiCol_Border,               0x00000000);
	ImGui::PushStyleColor(ImGuiCol_Button,               0xff282828);
	ImGui::PushStyleColor(ImGuiCol_ButtonActive,         0xff303030);
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered,        0xff3C3C3C);
	ImGui::PushStyleColor(ImGuiCol_WindowBg,             0xff141414);
	ImGui::PushStyleColor(ImGuiCol_PopupBg,              0xff141414);
	ImGui::PushStyleColor(ImGuiCol_FrameBg,              0xff322d23);
	ImGui::PushStyleColor(ImGuiCol_FrameBgActive,        0xff3c362a);
	ImGui::PushStyleColor(ImGuiCol_FrameBgHovered,       0xff4b4436);
	ImGui::PushStyleColor(ImGuiCol_TitleBg,              0xff000000);
	ImGui::PushStyleColor(ImGuiCol_TitleBgActive,        0xff000000);
	ImGui::PushStyleColor(ImGuiCol_Header,               0xff322d23);
	ImGui::PushStyleColor(ImGuiCol_HeaderActive,         0xff4a4A00);
	ImGui::PushStyleColor(ImGuiCol_HeaderHovered,        0xff5d5D00);
	ImGui::PushStyleColor(ImGuiCol_TableBorderLight,     0xff2D2D2D);
	ImGui::PushStyleColor(ImGuiCol_TableHeaderBg,        0xff0A0A0A);
	ImGui::PushStyleColor(ImGuiCol_ScrollbarBg,          0xff0A0A0A);
	ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab,        0xff373737);
	ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive,  0xff4B4B4B);
	ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, 0xff414141);
	ImGui::PushStyleColor(ImGuiCol_TabActive,            0xff404000);
	ImGui::PushStyleColor(ImGuiCol_TabHovered,           0xff808000);
	ImGui::PushStyleColor(ImGuiCol_Tab,                  0xff452b0d);
	ImGui::PushStyleColor(ImGuiCol_Separator,            0xff404000);
	
	ImGuiWindowFlags window_flags;
	if(popoutInspector){
		window_flags = ImGuiWindowFlags_None;
	}else{
		//resize tool menu if main menu bar is open
		ImGui::SetNextWindowSize(ImVec2(DeshWindow->width / 5, DeshWindow->height - (menubarheight + debugbarheight)));
		ImGui::SetNextWindowPos(ImVec2(0, menubarheight));
		window_flags = ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove
			| ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar;
	}
	ImGui::Begin("Inspector", (bool*)1, window_flags);
	
	//capture mouse if hovering over this window
	WinHovCheck;
	if(DeshInput->mouseX < ImGui::GetWindowPos().x + ImGui::GetWindowWidth()){
		WinHovFlag = true;
	}
	
	if(ImGui::BeginTabBar("MajorTabs")){
		if(ImGui::BeginTabItem("Entities")){
			EntitiesTab();
			ImGui::EndTabItem();
		}
		if(ImGui::BeginTabItem("Storage")){
			SetPadding;
			if(ImGui::BeginTabBar("StorageTabs")){
				if(ImGui::BeginTabItem("Meshes"))   { MeshesTab();    ImGui::EndTabItem(); }
				if(ImGui::BeginTabItem("Textures")) { TexturesTab();  ImGui::EndTabItem(); }
				if(ImGui::BeginTabItem("Materials")){ MaterialsTab(); ImGui::EndTabItem(); }
				if(ImGui::BeginTabItem("Models"))   { ModelsTab();    ImGui::EndTabItem(); }
				if(ImGui::BeginTabItem("Fonts"))    { /*FontsTab();*/ ImGui::EndTabItem(); }
				ImGui::EndTabBar();
			}
			ImGui::EndTabItem();
		}
		if(ImGui::BeginTabItem("Settings")){
			SettingsTab();
			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}
	
	ImGui::PopStyleVar(8);
	ImGui::PopStyleColor(24);
	ImGui::End();
}//inspector

///////////////////
//// @debugbar ////
///////////////////
void DebugBar(){
	//for getting fps
	ImGuiIO& io = ImGui::GetIO();
	
	//num of active columns
	int activecols = 0;
	
	//font size for centering ImGui::TextEx
	fontsize = ImGui::GetFontSize();
	
	//flags for showing different things
	persist bool show_fps = true;
	persist bool show_fps_graph = true;
	persist bool show_time = true;
	
	//window styling
	ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(0, 2));
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 0));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	ImGui::PushStyleColor(ImGuiCol_Border,           0x00000000);
	ImGui::PushStyleColor(ImGuiCol_WindowBg,         0xff141414);
	ImGui::PushStyleColor(ImGuiCol_TableBorderLight, 0xff2d2d2d);
	ImGui::SetNextWindowSize(ImVec2(DeshWindow->width, 20));
	ImGui::SetNextWindowPos(ImVec2(0, DeshWindow->height - 20));
	ImGui::Begin("DebugBar", (bool*)1, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
	debugbarheight = 20;
	//capture mouse if hovering over this window
	WinHovCheck;
	
	activecols = show_fps + show_fps_graph + show_time + 1;
	if(ImGui::BeginTable("DebugBarTable", activecols, ImGuiTableFlags_BordersV | ImGuiTableFlags_NoPadInnerX | ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_ContextMenuInBody | ImGuiTableFlags_SizingFixedFit)){
		ImGui::TableSetupColumn("FPS",            ImGuiTableColumnFlags_WidthFixed, 64);
		ImGui::TableSetupColumn("FPSGraphInline", ImGuiTableColumnFlags_WidthFixed, 64);
		ImGui::TableSetupColumn("MiddleSep",      ImGuiTableColumnFlags_WidthStretch, 0);
		ImGui::TableSetupColumn("Time",           ImGuiTableColumnFlags_WidthFixed, 64);
		
		//FPS
		int FPS = floor(io.Framerate);
		if(ImGui::TableNextColumn() && show_fps){
			ImGui::Text("FPS:%4d", FPS);
		}
		
		//FPS graph inline
		if(ImGui::TableNextColumn() && show_fps_graph){
			//how much data we store
			persist int prevstoresize = 100;
			persist int storesize = 100;
			
			//how often we update
			persist int fupdate = 20;
			persist int frame_count = 0;
			
			//maximum FPS
			persist int maxval = 0;
			
			//real values and printed values
			persist std::vector<float> values(storesize);
			persist std::vector<float> pvalues(storesize);
			
			//dynamic resizing that may get removed later if it sucks
			//if FPS finds itself as less than half of what the max used to be we lower the max
			if(FPS > maxval || FPS < maxval / 2){
				maxval = FPS;
			}
			
			//if changing the amount of data we're storing we have to reverse
			//each data set twice to ensure the data stays in the right place when we move it
			if(prevstoresize != storesize){
				std::reverse(values.begin(), values.end());    values.resize(storesize);  std::reverse(values.begin(), values.end());
				std::reverse(pvalues.begin(), pvalues.end());  pvalues.resize(storesize); std::reverse(pvalues.begin(), pvalues.end());
				prevstoresize = storesize;
			}
			
			std::rotate(values.begin(), values.begin() + 1, values.end());
			
			//update real set if we're not updating yet or update the graph if we are
			if(frame_count < fupdate){
				values[values.size() - 1] = FPS;
				frame_count++;
			}
			else{
				float avg = Math::average(values.begin(), values.end(), storesize);
				std::rotate(pvalues.begin(), pvalues.begin() + 1, pvalues.end());
				pvalues[pvalues.size() - 1] = std::floorf(avg);
				
				frame_count = 0;
			}
			
			ImGui::PushStyleColor(ImGuiCol_PlotLines, ImGui::ColorToImVec4(color(0, 255, 200, 255)));
			ImGui::PushStyleColor(ImGuiCol_FrameBg, ImGui::ColorToImVec4(color(20, 20, 20, 255)));
			
			ImGui::PlotLines("", &pvalues[0], pvalues.size(), 0, 0, 0, maxval, ImVec2(64, 20));
			
			ImGui::PopStyleColor();
			ImGui::PopStyleColor();
		}
		
		//Middle Empty ImGui::Separator (alert box)
		if(ImGui::TableNextColumn()){
			if(DeshConsole->show_alert){
				f32 flicker = (sinf(M_2PI * DeshTotalTime + cosf(M_2PI * DeshTotalTime)) + 1) / 2;
				color col_bg = DeshConsole->alert_color * flicker;    col_bg.a = 255;
				color col_text = DeshConsole->alert_color * -flicker; col_text.a = 255;
				
				ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, ImGui::GetColorU32(ImGui::ColorToImVec4(col_bg)));
				
				string str6;
				if(DeshConsole->alert_count > 1){
					str6 = TOSTRING("(", DeshConsole->alert_count, ") ", string(DeshConsole->alert_message.c_str()));
				}else{
					str6 = string(DeshConsole->alert_message.c_str());
				}
				float strlen6 = (fontw) * str6.count;
				ImGui::SameLine((ImGui::GetColumnWidth() - strlen6) / 2); ImGui::PushItemWidth(-1);
				ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorToImVec4(color(col_text)));
				ImGui::TextEx(str6.str);
				ImGui::PopStyleColor();
			}
		}
		
		//Show Time
		if(ImGui::TableNextColumn()){
			string str7 = DeshTime->FormatDateTime("{h}:{m}:{s}").c_str();
			float strlen7 = fontw * str7.count;
			ImGui::SameLine(32 - (strlen7 / 2));
			ImGui::TextEx(str7.str);
		}
		
		//Context menu for toggling parts of the bar
		if(ImGui::IsMouseReleased(1) && ImGui::IsWindowHovered()) ImGui::OpenPopup("Context");
		if(ImGui::BeginPopup("Context")){
			DeshConsole->IMGUI_MOUSE_CAPTURE = true;
			ImGui::Separator();
			if(ImGui::Button("Open Debug Menu")){
				//showDebugTools = true;
				ImGui::CloseCurrentPopup();
			}
			
			ImGui::EndPopup();
		}
		ImGui::EndTable();
	}
	
	ImGui::PopStyleVar(3);
	ImGui::PopStyleColor(3);
	ImGui::End();
}


/////////////////////
//// @debugtimes ////
/////////////////////
//void Editor::DrawTimes(){
//    std::string time1 = DeshTime->FormatTickTime("Time       : {t}\n"
//        "Window     : {w}\n"
//        "Input      : {i}\n");
//    time1 += DeshAdmin->FormatAdminTime("Physics Lyr: {P}\n"
//        "        Sys: {p}\n"
//        "Canvas  Lyr: {C}\n"
//        "        Sys: {c}\n"
//        "World   Lyr: {W}\n"
//        "        Sys: {w}\n"
//        "Sound   Lyr: {S}\n"
//        "Sound   Lyr: {S}\n"
//        "        Sys: {s}\n"
//        "Editor     : {e}\n");
//    time1 += DeshTime->FormatTickTime("Admin      : {a}\n"
//        "Console    : {c}\n"
//        "Render     : {r}\n"
//        "Frame      : {f}");
//
//    ImGui::SetCursorPos(ImVec2(DeshWindow->width - 150, menubarheight));
//    ImGui::TextEx(time1.str);
//}


///////////////
//// @grid ////
///////////////
void WorldGrid(){
	int lines = 100;
	f32 xp = floor(AtmoAdmin->camera.position.x) + lines;
	f32 xn = floor(AtmoAdmin->camera.position.x) - lines;
	f32 zp = floor(AtmoAdmin->camera.position.z) + lines;
	f32 zn = floor(AtmoAdmin->camera.position.z) - lines;
	
	color color(50, 50, 50);
	for(int i = 0; i < lines * 2 + 1; i++){
		vec3 v1 = vec3(xn + i, 0, zn);
		vec3 v2 = vec3(xn + i, 0, zp);
		vec3 v3 = vec3(xn, 0, zn + i);
		vec3 v4 = vec3(xp, 0, zn + i);
		
		if(xn + i != 0) Render::DrawLine(v1, v2, color);
		if(zn + i != 0) Render::DrawLine(v3, v4, color);
	}
	
	Render::DrawLine(vec3{ -1000,0,0 }, vec3{ 1000,0,0 }, Color_Red);
	Render::DrawLine(vec3{ 0,-1000,0 }, vec3{ 0,1000,0 }, Color_Green);
	Render::DrawLine(vec3{ 0,0,-1000 }, vec3{ 0,0,1000 }, Color_Blue);
}

///////////////
//// @axis ////
///////////////
void ShowWorldAxis(){
	vec3
		x = AtmoAdmin->camera.position + AtmoAdmin->camera.forward + vec3::RIGHT * 0.1,
	y = AtmoAdmin->camera.position + AtmoAdmin->camera.forward + vec3::UP * 0.1,
	z = AtmoAdmin->camera.position + AtmoAdmin->camera.forward + vec3::FORWARD * 0.1;
	
	vec2
		spx = Math::WorldToScreen2(x, AtmoAdmin->camera.projMat, AtmoAdmin->camera.viewMat, DeshWindow->dimensions) - DeshWindow->dimensions / 2,
	spy = Math::WorldToScreen2(y, AtmoAdmin->camera.projMat, AtmoAdmin->camera.viewMat, DeshWindow->dimensions) - DeshWindow->dimensions / 2,
	spz = Math::WorldToScreen2(z, AtmoAdmin->camera.projMat, AtmoAdmin->camera.viewMat, DeshWindow->dimensions) - DeshWindow->dimensions / 2;
	
	vec2 offset = vec2(DeshWindow->width - 50, DeshWindow->height - debugbarheight - 50);
	
	Render::DrawLineUI(offset, spx + offset, 1, Color_Red);
	Render::DrawLineUI(offset, spy + offset, 1, Color_Green);
	Render::DrawLineUI(offset, spz + offset, 1, Color_Blue);
}

//////////////////////////@@
//// @transform gizmo ////TODO: local vs world, mouse offset
//////////////////////////TODO: plane translate, axis rotation, free rotation, axis scale, free scale 
local b32 TransformGizmo(){
	enum TransformType{
		TransformType_NONE,
		TransformType_TranslateSelectAxis,
		TransformType_TranslateX,
		TransformType_TranslateY,
		TransformType_TranslateZ,
		TransformType_TranslateXY,
		TransformType_TranslateXZ,
		TransformType_TranslateYZ,
		TransformType_TranslateFree,
		TransformType_RotateSelectAxis,
		TransformType_RotateX,
		TransformType_RotateY,
		TransformType_RotateZ,
		TransformType_RotateFree,
		TransformType_ScaleSelectAxis,
		TransformType_ScaleX,
		TransformType_ScaleY,
		TransformType_ScaleZ,
		TransformType_ScaleFree,
	};
	persist Type action = TransformType_NONE;
	persist vec2 mouse_offset = vec2::ZERO;
	persist vec3 initial = vec3::ZERO;
	persist f32  initial_dist = 0.0f;
	persist b32  dragging = false;
	persist b32  select = false;
	persist b32  undo = false;
	persist b32  local_space = false;
	
	//early out if no selected entities to transform
	if(selected_entities.count == 0){
		action = TransformType_NONE;
		return false;
	}
	
	//change transform type
	if(DeshInput->KeyPressed(Key::ESCAPE)) action = TransformType_NONE;
	if(!DeshInput->KeyDown(MouseButton::RIGHT)){
		if(DeshInput->KeyPressed(AtmoAdmin->controller.transformTranslate)) action = TransformType_TranslateSelectAxis;
		if(DeshInput->KeyPressed(AtmoAdmin->controller.transformRotate))    action = TransformType_RotateSelectAxis;
		if(DeshInput->KeyPressed(AtmoAdmin->controller.transformScale))     action = TransformType_ScaleSelectAxis;
	}
	
	//change axis
	if      (action >= TransformType_TranslateSelectAxis && action <= TransformType_TranslateFree){
		if      (DeshInput->KeyPressed(Key::X)){
			if      (DeshInput->KeyPressed(Key::Y)){
				action = TransformType_TranslateXY;
			}else if(DeshInput->KeyPressed(Key::Z)){
				action = TransformType_TranslateXZ;
			}else{
				action = TransformType_TranslateX;
			}
		}else if(DeshInput->KeyPressed(Key::Y)){
			if      (DeshInput->KeyPressed(Key::X)){
				action = TransformType_TranslateXY;
			}else if(DeshInput->KeyPressed(Key::Z)){
				action = TransformType_TranslateYZ;
			}else{
				action = TransformType_TranslateY;
			}
		}else if(DeshInput->KeyPressed(Key::Z)){
			if      (DeshInput->KeyPressed(Key::X)){
				action = TransformType_TranslateXZ;
			}else if(DeshInput->KeyPressed(Key::Y)){
				action = TransformType_TranslateYZ;
			}else{
				action = TransformType_TranslateZ;
			}
		}else if(DeshInput->KeyPressed(Key::F)){
			action = TransformType_TranslateFree;
		}
	}else if(action >= TransformType_RotateSelectAxis && action <= TransformType_RotateFree){
		if      (DeshInput->KeyPressed(Key::X)){
			action = TransformType_RotateX;
		}else if(DeshInput->KeyPressed(Key::Y)){
			action = TransformType_RotateY;
		}else if(DeshInput->KeyPressed(Key::Z)){
			action = TransformType_RotateZ;
		}else if(DeshInput->KeyPressed(Key::F)){
			action = TransformType_RotateFree;
		}
	}else if(action >= TransformType_ScaleSelectAxis && action <= TransformType_ScaleFree){
		if      (DeshInput->KeyPressed(Key::X)){
			action = TransformType_ScaleX;
		}else if(DeshInput->KeyPressed(Key::Y)){
			action = TransformType_ScaleY;
		}else if(DeshInput->KeyPressed(Key::Z)){
			action = TransformType_ScaleZ;
		}else if(DeshInput->KeyPressed(Key::F)){
			action = TransformType_ScaleFree;
		}
	}
	
	//mouse input
	if(!WinHovFlag){
		if(DeshInput->KeyPressed(MouseButton::LEFT)){
			select = true;
			dragging = true;
		}else if(DeshInput->KeyDown(MouseButton::LEFT)){
			select = false;
			dragging = true;
		}else if(DeshInput->KeyReleased(MouseButton::LEFT)){
			select = false;
			dragging = false;
			undo = true;
		}
	}
	
	//dragging and drawing
	Entity* sel = selected_entities[0];
	Camera* cam = &AtmoAdmin->camera;
	vec3 sel_pos     = sel->transform.position;
	vec3 sel_rot     = sel->transform.rotation;
	vec3 sel_scale   = sel->transform.scale;
	vec3 mouse_world = Math::ScreenToWorld(DeshInput->mousePos, cam->projMat, cam->viewMat, DeshWindow->dimensions);
	f32  cam_dist    = sel_pos.distanceTo(cam->position);
	f32  draw_scale  = cam_dist / 12.f;
	b32  hit_gizmo   = false;
	switch(action){
		//// translation ////
		case TransformType_TranslateSelectAxis:{
			vec3 ray_dir = (mouse_world - cam->position).normalized();
			vec3 aabb_min = sel_pos+vec3{draw_scale,0,0}-vec3{2.f,.1f,.1f}*draw_scale/2;
			vec3 aabb_free = vec3::ONE*draw_scale/2.f;
			if(   (DeshInput->mouseX > 0) && (DeshInput->mouseY > 0) 
			   && (DeshInput->mouseX < DeshWindow->screenWidth) && (DeshInput->mouseY < DeshWindow->screenHeight)){
				//x axis
				if(AABBRaycast(mouse_world, ray_dir, aabb_min, sel_pos+vec3{2.f,.1f,.1f}*draw_scale*2)){
					Render::DrawBoxFilled(mat4::TransformationMatrix(sel_pos+vec3{draw_scale,0,0}, vec3::ZERO, vec3{2.f,.1f,.1f}*draw_scale), Color_Yellow);
					if(DeshInput->KeyPressed(MouseButton::LEFT)){
						action = TransformType_TranslateX;
						hit_gizmo = true;
					}
				}else{
					Render::DrawBoxFilled(mat4::TransformationMatrix(sel_pos+vec3{draw_scale,0,0}, vec3::ZERO, vec3{2.f,.1f,.1f}*draw_scale), Color_Red);
				}
				
				//y axis
				if(AABBRaycast(mouse_world, ray_dir, aabb_min, sel_pos+vec3{.1f,2.f,.1f}*draw_scale*2)){
					Render::DrawBoxFilled(mat4::TransformationMatrix(sel_pos+vec3{0,draw_scale,0}, vec3::ZERO, vec3{.1f,2.f,.1f}*draw_scale), Color_Yellow);
					if(DeshInput->KeyPressed(MouseButton::LEFT)){
						action = TransformType_TranslateY;
						hit_gizmo = true;
					}
				}else{
					Render::DrawBoxFilled(mat4::TransformationMatrix(sel_pos+vec3{0,draw_scale,0}, vec3::ZERO, vec3{.1f,2.f,.1f}*draw_scale), Color_Green);
				}
				
				//z axis
				if(AABBRaycast(mouse_world, ray_dir, aabb_min, sel_pos+vec3{.1f,.1f,2.f}*draw_scale*2)){
					Render::DrawBoxFilled(mat4::TransformationMatrix(sel_pos+vec3{0,0,draw_scale}, vec3::ZERO, vec3{.1f,.1f,2.f}*draw_scale), Color_Yellow);
					if(DeshInput->KeyPressed(MouseButton::LEFT)){
						action = TransformType_TranslateZ;
						hit_gizmo = true;
					}
				}else{
					Render::DrawBoxFilled(mat4::TransformationMatrix(sel_pos+vec3{0,0,draw_scale}, vec3::ZERO, vec3{.1f,.1f,2.f}*draw_scale), Color_Blue);
				}
				
				//free axis
				if(AABBRaycast(mouse_world, ray_dir, sel_pos-aabb_free, sel_pos+aabb_free)){
					Render::DrawBoxFilled(mat4::TransformationMatrix(sel_pos, vec3::ZERO, vec3::ONE*draw_scale), Color_Yellow);
					if(DeshInput->KeyPressed(MouseButton::LEFT)){
						action = TransformType_TranslateFree;
						hit_gizmo = true;
					}
				}else{
					Render::DrawBoxFilled(mat4::TransformationMatrix(sel_pos, vec3::ZERO, vec3::ONE*draw_scale), Color_LightGrey);
				}
			}
		}break;
		case TransformType_TranslateX:{
			if(select){
				initial = sel_pos;
				initial_dist = cam_dist;
			}else if(dragging){
				mouse_world = ((mouse_world - cam->position).normalized() * 1000.f) + cam->position;
				if(Math::AngBetweenVectors(cam->forward.yZero(), cam->forward) > 60.f){
					sel->transform.position.x = Math::VectorPlaneIntersect(initial, vec3::UP, cam->position, mouse_world).x;
					if(sel->physics) sel->physics->position.x = sel->transform.position.x;
				}else{
					sel->transform.position.x = Math::VectorPlaneIntersect(initial, vec3::FORWARD, cam->position, mouse_world).x;
					if(sel->physics) sel->physics->position.x = sel->transform.position.x;
				}
			}else if(undo){
				AddUndoTranslate(&sel->transform, &initial, &sel_pos);
			}
			Render::DrawBoxFilled(mat4::TransformationMatrix(sel_pos+vec3{draw_scale,0,0}, vec3::ZERO, vec3{2.f,.1f,.1f}*draw_scale), Color_Red);
		}break;
		case TransformType_TranslateY:{
			if(select){
				initial = sel_pos;
				initial_dist = cam_dist;
			}else if(dragging){
				mouse_world = ((mouse_world - cam->position).normalized() * 1000.f) + cam->position;
				if(Math::AngBetweenVectors(cam->forward.yZero(), cam->forward) > 60.f){
					sel->transform.position.y = Math::VectorPlaneIntersect(initial, vec3::RIGHT, cam->position, mouse_world).y;
					if(sel->physics) sel->physics->position.y = sel->transform.position.y;
				}else{
					sel->transform.position.y = Math::VectorPlaneIntersect(initial, vec3::FORWARD, cam->position, mouse_world).y;
					if(sel->physics) sel->physics->position.y = sel->transform.position.y;
				}
			}else if(undo){
				AddUndoTranslate(&sel->transform, &initial, &sel_pos);
			}
			Render::DrawBoxFilled(mat4::TransformationMatrix(sel_pos+vec3{0,draw_scale,0}, vec3::ZERO, vec3{.1f,2.f,.1f}*draw_scale), Color_Green);
		}break;
		case TransformType_TranslateZ:{
			if(select){
				initial = sel_pos;
				initial_dist = cam_dist;
			}else if(dragging){
				mouse_world = ((mouse_world - cam->position).normalized() * 1000.f) + cam->position;
				if(Math::AngBetweenVectors(cam->forward.yZero(), cam->forward) > 60.f){
					sel->transform.position.z = Math::VectorPlaneIntersect(initial, vec3::UP, cam->position, mouse_world).z;
					if(sel->physics) sel->physics->position.z = sel->transform.position.z;
				}else{
					sel->transform.position.z = Math::VectorPlaneIntersect(initial, vec3::RIGHT, cam->position, mouse_world).z;
					if(sel->physics) sel->physics->position.z = sel->transform.position.z;
				}
			}else if(undo){
				AddUndoTranslate(&sel->transform, &initial, &sel_pos);
			}
			Render::DrawBoxFilled(mat4::TransformationMatrix(sel_pos+vec3{0,0,draw_scale}, vec3::ZERO, vec3{.1f,.1f,2.f}*draw_scale), Color_Blue);
		}break;
		case TransformType_TranslateFree:{
			if      (DeshInput->KeyPressed(MouseButton::SCROLLUP)){
				initial_dist += 1.f;
			}else if(DeshInput->KeyPressed(MouseButton::SCROLLDOWN)){
				initial_dist -= 1.f;
			}
			
			if(select){
				initial = sel_pos;
				initial_dist = cam_dist;
			}else if(dragging){
				sel->transform.position = ((mouse_world - cam->position).normalized() * initial_dist) + cam->position;
				if(sel->physics) sel->physics->position = sel->transform.position;
			}else if(undo){
				AddUndoTranslate(&sel->transform, &initial, &sel_pos);
			}
			Render::DrawBoxFilled(mat4::TransformationMatrix(sel_pos, vec3::ZERO, vec3::ONE*draw_scale), Color_LightGrey);
		}break;
		
		//// rotation ////
		
		//// scale ////
	}
	
	return hit_gizmo;
}

///////////////
//// @init ////
///////////////
void Editor::Init(){
	selected_entities.reserve(8);
	
	popoutInspector = false;
	showInspector = true;
	showTimes = true;
	showDebugBar = true;
	showMenuBar = true;
	showImGuiDemoWindow = false;
	showDebugLayer = true;
	showWorldGrid = true;
	
	dir_levels   = Assets::iterateDirectory(Assets::dirLevels());
	dir_meshes   = Assets::iterateDirectory(Assets::dirModels(), ".mesh");
	dir_textures = Assets::iterateDirectory(Assets::dirTextures());
	dir_models   = Assets::iterateDirectory(Assets::dirModels(), ".obj");
	dir_fonts    = Assets::iterateDirectory(Assets::dirFonts());
}

/////////////////
//// @update ////
/////////////////
void Editor::Update(){
	fonth = ImGui::GetFontSize();
	fontw = fonth / 2.f;
	
	
	///////////////
	//// input ////
	///////////////
	//// simulate physics in editor ////
	if(DeshInput->KeyPressed(Key::P | InputMod_Lctrl)) AtmoAdmin->simulateInEditor = !AtmoAdmin->simulateInEditor;
	
	//// select ////
	b32 hit_gizmo = TransformGizmo();
	if(!hit_gizmo && !WinHovFlag && DeshInput->KeyPressed(MouseButton::LEFT)){
		//NOTE adjusting the projection matrix so the nearZ is at least .1, produces bad results if less
		mat4 adjusted_proj = Camera::MakePerspectiveProjectionMatrix(DeshWindow->width, DeshWindow->height, AtmoAdmin->camera.fov, 
																	 AtmoAdmin->camera.farZ, Max(.1, AtmoAdmin->camera.nearZ));
		vec3 direction = (Math::ScreenToWorld(DeshInput->mousePos, adjusted_proj, AtmoAdmin->camera.viewMat, DeshWindow->dimensions) 
						  - AtmoAdmin->camera.position).normalized();
		
		if(Entity* e = AtmoAdmin->EntityRaycast(AtmoAdmin->camera.position, direction, AtmoAdmin->camera.farZ)){
			if      (DeshInput->ShiftDown()){ //add new selected
				selected_entities.add(e);
			}else if(DeshInput->CtrlDown()){  //remove selected
				forI(selected_entities.count){
					if(selected_entities[i] == e){
						selected_entities.remove(i);
						break;
					}
				}
			}else{                            //clear selected, add selected
				selected_entities.clear();
				selected_entities.add(e);
			}
		}else{
			selected_entities.clear();
		}
	}
	
	//// render ////
	if(DeshInput->KeyPressed(Key::F5)) Render::ReloadAllShaders();
	
	//fullscreen toggle
	if(DeshInput->KeyPressed(Key::F11)){
		if(DeshWindow->displayMode == DisplayMode_Windowed || DeshWindow->displayMode == DisplayMode_Borderless)
			DeshWindow->UpdateDisplayMode(DisplayMode_Fullscreen);
		else 
			DeshWindow->UpdateDisplayMode(DisplayMode_Windowed);
	}
	
	//// camera ////
	//uncomment once ortho has been implemented again
	//persist vec3 ogpos;
	//persist vec3 ogrot;
	//if(DeshInput->KeyPressed(AtmoAdmin->controller.perspectiveToggle)){
	//	switch (camera->mode){
	//	case(CameraMode_Perspective): {
	//		ogpos = camera->position;
	//		ogrot = camera->rotation;
	//		camera->mode = CameraMode_Orthographic;
	//		camera->farZ = 1000000;
	//	} break;
	//	case(CameraMode_Orthographic): {
	//		camera->position = ogpos;
	//		camera->rotation = ogrot;
	//		camera->mode = CameraMode_Perspective;
	//		camera->farZ = 1000;
	//		camera->UpdateProjectionMatrix();
	//	} break;
	//	}
	//}
	//
	////ortho views
	//if      (DeshInput->KeyPressed(AtmoAdmin->controller.orthoFrontView))    camera->orthoview = OrthoView_Front;
	//else if(DeshInput->KeyPressed(AtmoAdmin->controller.orthoBackView))     camera->orthoview = OrthoView_Back;
	//else if(DeshInput->KeyPressed(AtmoAdmin->controller.orthoRightView))    camera->orthoview = OrthoView_Right;
	//else if(DeshInput->KeyPressed(AtmoAdmin->controller.orthoLeftView))     camera->orthoview = OrthoView_Left;
	//else if(DeshInput->KeyPressed(AtmoAdmin->controller.orthoTopDownView))  camera->orthoview = OrthoView_Top;
	//else if(DeshInput->KeyPressed(AtmoAdmin->controller.orthoBottomUpView)) camera->orthoview = OrthoView_Bottom;
	
	if(DeshInput->KeyPressed(AtmoAdmin->controller.gotoSelected)){
		AtmoAdmin->camera.position = selected_entities[0]->transform.position + vec3(4.f, 3.f, -4.f);
		AtmoAdmin->camera.rotation = { 28.f, -45.f, 0.f };
	}
	
	//// interface ////
	if(DeshInput->KeyPressed(AtmoAdmin->controller.toggleConsole))   DeshConsole->dispcon = !DeshConsole->dispcon;
	if(DeshInput->KeyPressed(AtmoAdmin->controller.toggleDebugMenu)) showInspector = !showInspector;
	if(DeshInput->KeyPressed(AtmoAdmin->controller.toggleDebugBar))  showDebugBar = !showDebugBar;
	if(DeshInput->KeyPressed(AtmoAdmin->controller.toggleMenuBar))   showMenuBar = !showMenuBar;
	
	//// cut/copy/paste ////
	if(DeshInput->KeyPressed(AtmoAdmin->controller.cut))   CutEntities();
	if(DeshInput->KeyPressed(AtmoAdmin->controller.copy))  CopyEntities();
	if(DeshInput->KeyPressed(AtmoAdmin->controller.paste)) PasteEntities();
	
	//// undo/redo ////
	if(DeshInput->KeyPressed(AtmoAdmin->controller.undo)) Undo();
	if(DeshInput->KeyPressed(AtmoAdmin->controller.redo)) Redo();
	
	//// save/load level ////
	if(DeshInput->KeyPressed(AtmoAdmin->controller.saveLevel) && !DeshInput->KeyDown(MouseButton::RIGHT)){
		if(AtmoAdmin->levelName){
			AtmoAdmin->SaveLevel(cstring{AtmoAdmin->levelName.str,(u64)AtmoAdmin->levelName.count});
		}else{;
			LogW("editor","Level not saved before; Use 'Save As'");
		}
	}
	
	///////////////////
	//// interface ////
	///////////////////
	//program crashes somewhere in Inpector() if minimized
	if(!DeshWindow->minimized){
		WinHovFlag = 0;
		
		//if(showDebugLayer) DebugLayer();
		//if(showTimes)      DrawTimes();
		if(showInspector)  Inspector();
		if(showDebugBar)   DebugBar();
		if(showMenuBar)    MenuBar();
		if(showWorldGrid)  WorldGrid();
		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 1)); {
			if(showImGuiDemoWindow) ImGui::ShowDemoWindow();
		}ImGui::PopStyleColor();
		
		ShowWorldAxis();
		if(!showMenuBar)   menubarheight = 0;
		if(!showDebugBar)  debugbarheight = 0;
	}
}

////////////////
//// @reset ////
////////////////
void Editor::Reset(){
	selected_entities.clear();
	undos.clear();
	redos.clear();
}

//////////////////
//// @cleanup ////
//////////////////
void Editor::Cleanup(){
	
}