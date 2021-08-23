#include "Editor.h"
#include "admin.h"
#include "attributes/ModelInstance.h"
#include "camerainstance.h"
#include "entities/Entity.h"
#include "core/assets.h"
#include "core/console.h"
#include "core/imgui.h"
#include "core/renderer.h"
#include "core/window.h"
#include "core/input.h"
#include "core/time.h"
#include "core/storage.h"
#include "utils/array.h"
#include "math/Math.h"
#include "geometry/geometry.h"
#include "geometry/Edge.h"

///////////////////////
//// @undo structs ////
///////////////////////
enum EditActionType_{
	EditActionType_NONE, 
	EditActionType_Select, 
	EditActionType_Translate, 
	EditActionType_Rotate, 
	EditActionType_Scale, 
	EditActionType_Create, 
	EditActionType_Delete,
	EditActionType_COUNT,
}; typedef u32 EditActionType;

struct EditAction{ //48 bytes
	EditActionType type;
	u32 data[11];
};

////////////////////
//// @undo vars ////
////////////////////
//TODO(delle) handle going over MEMORY_LIMIT
//TODO(delle,Op) maybe use a vector with fixed size and store redos at back and use swap rather than construction/deletion
local u64 MEMORY_LIMIT = Megabytes(8); //8MB = ~1 million undos
#include <deque>
local std::deque<EditAction> undos = std::deque<EditAction>();
local std::deque<EditAction> redos = std::deque<EditAction>();


/////////////////////
//// @undo funcs ////
/////////////////////
//select data layout:
//0x00  void*      | selected entity pointer
//0x08  void*      | old selection
//0x10  void*      | new selection
void AddUndoSelect(void** sel, void* oldEnt, void* newEnt){
	Assert(sizeof(u32)*2 == sizeof(void*), "assume ptr is 8 bytes");
	EditAction edit; edit.type = EditActionType_Select;
	memcpy(edit.data + 0, &sel,    sizeof(u32)*2);
	memcpy(edit.data + 2, &oldEnt, sizeof(u32)*2);
	memcpy(edit.data + 4, &newEnt, sizeof(u32)*2);
	undos.push_back(edit);
	redos.clear();
}
void UndoSelect(EditAction* edit){
	void** sel;
	memcpy(&sel, ((u32*)edit->data) + 0, sizeof(u32)*2);
	memcpy( sel, ((u32*)edit->data) + 2, sizeof(u32)*2);
}
void RedoSelect(EditAction* edit){
	void** sel;
	memcpy(&sel, ((u32*)edit->data) + 0, sizeof(u32)*2);
	memcpy( sel, ((u32*)edit->data) + 4, sizeof(u32)*2);
}

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

//create data layout:
void AddUndoCreate(){
	
}
void UndoCreate(EditAction* edit){
	
}
void RedoCreate(EditAction* edit){
	
}

//delete data layout:
void AddUndoDelete(){
	
}
void UndoDelete(EditAction* edit){
	
}
void RedoDelete(EditAction* edit){
	
}

void Undo(u32 count = 1){
	forI((count < undos.size()) ? count : undos.size()){
		u32 n = undos.size()-i-1;
		switch(undos[n].type){
			case(EditActionType_Select):   { UndoSelect(&undos[n]);    }break;
			case(EditActionType_Translate):{ UndoTranslate(&undos[n]); }break;
			case(EditActionType_Rotate):   { UndoRotate(&undos[n]);    }break;
			case(EditActionType_Scale):    { UndoScale(&undos[n]);     }break;
			case(EditActionType_Create):   { UndoCreate(&undos[n]);    }break;
			case(EditActionType_Delete):   { UndoDelete(&undos[n]);    }break;
		}
		redos.push_back(undos.back());
		undos.pop_back();
	}
}

void Redo(u32 count = 1){
	forI((count < redos.size()) ? count : redos.size()){
		u32 n = redos.size()-i-1;
		switch(redos[n].type){
			case(EditActionType_Select):   { RedoSelect(&redos[n]);    }break;
			case(EditActionType_Translate):{ RedoTranslate(&redos[n]); }break;
			case(EditActionType_Rotate):   { RedoRotate(&redos[n]);    }break;
			case(EditActionType_Scale):    { RedoScale(&redos[n]);     }break;
			case(EditActionType_Create):   { RedoCreate(&redos[n]);    }break;
			case(EditActionType_Delete):   { RedoDelete(&redos[n]);    }break;
		}
		undos.push_back(redos.back());
		redos.pop_back();
	}
}

//////////////////////
//// @editor vars ////
//////////////////////
local array<Entity*> selected_entities;
local CameraInstance* editor_camera;
local string level_name;

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

local bool popoutInspector;
local bool showInspector;
local bool showTimes;
local bool showDebugBar;
local bool showMenuBar;
local bool showImGuiDemoWindow;
local bool showDebugLayer;
local bool showWorldGrid;

//current palette:
//https://lospec.com/palette-list/slso8
//TODO(sushi, Ui) implement menu style file loading sort of stuff yeah
//TODO(sushi, Ui) standardize what UI element each color belongs to
local struct {
	color c1 = color(0x0d2b45); //midnight blue
	color c2 = color(0x203c56); //dark gray blue
	color c3 = color(0x544e68); //purple gray
	color c4 = color(0x8d697a); //pink gray
	color c5 = color(0xd08159); //bleached orange
	color c6 = color(0xffaa5e); //above but brighter
	color c7 = color(0xffd4a3); //skin white
	color c8 = color(0xffecd6); //even whiter skin
	color c9 = color(0x141414); //almost black
}colors_;

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
    
	void DebugDrawCircle(vec2 pos, float radius, color color = color::WHITE){
		ImGui::GetBackgroundDrawList()->AddCircle(ImVec2(pos.x, pos.y), radius, ImGui::GetColorU32(ImGui::ColorToImVec4(color)));
	}
    
	void DebugDrawCircle3(vec3 pos, float radius, color color = color::WHITE){
		CameraInstance* c = &AtmoAdmin->camera;
		vec2 windimen = DeshWindow->dimensions;
		vec2 pos2 = Math::WorldToScreen2(pos, c->projMat, c->viewMat, windimen);
		ImGui::GetBackgroundDrawList()->AddCircle(ImGui::vec2ToImVec2(pos2), radius, ImGui::GetColorU32(ImGui::ColorToImVec4(color)));
	}
    
	void DebugDrawCircleFilled3(vec3 pos, float radius, color color = color::WHITE){
		CameraInstance* c = &AtmoAdmin->camera;
		vec2 windimen = DeshWindow->dimensions;
		vec2 pos2 = Math::WorldToScreen2(pos, c->projMat, c->viewMat, windimen);
		ImGui::GetBackgroundDrawList()->AddCircleFilled(ImGui::vec2ToImVec2(pos2), radius, ImGui::GetColorU32(ImGui::ColorToImVec4(color)));
	}
    
	void DebugDrawLine(vec2 pos1, vec2 pos2, color color = color::WHITE){
		Math::ClipLineToBorderPlanes(pos1, pos2, DeshWindow->dimensions);
		ImGui::GetBackgroundDrawList()->AddLine(ImGui::vec2ToImVec2(pos1), ImGui::vec2ToImVec2(pos2), ImGui::GetColorU32(ImGui::ColorToImVec4(color)));
	}
    
	void DebugDrawLine3(vec3 pos1, vec3 pos2, color color = color::WHITE){
		CameraInstance* c = &AtmoAdmin->camera;
		vec2 windimen = DeshWindow->dimensions;
        
		vec3 pos1n = Math::WorldToCamera3(pos1, c->viewMat);
		vec3 pos2n = Math::WorldToCamera3(pos2, c->viewMat);
        
		if(Math::ClipLineToZPlanes(pos1n, pos2n, c->nearZ, c->farZ)){
			ImGui::GetBackgroundDrawList()->AddLine(ImGui::vec2ToImVec2(Math::CameraToScreen2(pos1n, c->projMat, windimen)),
                                                    ImGui::vec2ToImVec2(Math::CameraToScreen2(pos2n, c->projMat, windimen)),
                                                    ImGui::GetColorU32(ImGui::ColorToImVec4(color)));
		}
	}
    
	void DebugDrawText(const char* text, vec2 pos, color color = color::WHITE){
		ImGui::SetCursorPos(ImGui::vec2ToImVec2(pos));
        
		ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorToImVec4(color));
		ImGui::TextEx(text);
		ImGui::PopStyleColor();
	}
    
	void DebugDrawText3(const char* text, vec3 pos, color color = color::WHITE, vec2 twoDoffset = vec2::ZERO){
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
    
	void DebugDrawTriangle(vec2 p1, vec2 p2, vec2 p3, color color = color::WHITE){
		DebugDrawLine(p1, p2);
		DebugDrawLine(p2, p3);
		DebugDrawLine(p3, p1);
	}
    
	void DebugFillTriangle(vec2 p1, vec2 p2, vec2 p3, color color = color::WHITE){
		ImGui::GetBackgroundDrawList()->AddTriangleFilled(ImGui::vec2ToImVec2(p1), ImGui::vec2ToImVec2(p2), ImGui::vec2ToImVec2(p3),
                                                          ImGui::GetColorU32(ImGui::ColorToImVec4(color)));
	}
    
	void DebugDrawTriangle3(vec3 p1, vec3 p2, vec3 p3, color color = color::WHITE){
		DebugDrawLine3(p1, p2, color);
		DebugDrawLine3(p2, p3, color);
		DebugDrawLine3(p3, p1, color);
	}
    
	//TODO(sushi, Ui) add triangle clipping to this function
	void DebugFillTriangle3(vec3 p1, vec3 p2, vec3 p3, color color = color::WHITE){
		vec2 p1n = Math::WorldToScreen(p1, AtmoAdmin->camera.projMat, AtmoAdmin->camera.viewMat, DeshWindow->dimensions).toVec2();
		vec2 p2n = Math::WorldToScreen(p2, AtmoAdmin->camera.projMat, AtmoAdmin->camera.viewMat, DeshWindow->dimensions).toVec2();
		vec2 p3n = Math::WorldToScreen(p3, AtmoAdmin->camera.projMat, AtmoAdmin->camera.viewMat, DeshWindow->dimensions).toVec2();
        
		ImGui::GetBackgroundDrawList()->AddTriangleFilled(ImGui::vec2ToImVec2(p1n), ImGui::vec2ToImVec2(p2n), ImGui::vec2ToImVec2(p3n),
                                                          ImGui::GetColorU32(ImGui::ColorToImVec4(color)));
	}
    
	void DebugDrawGraphFloat(vec2 pos, float inval, float sizex = 100, float sizey = 100){
		//display in value
		ImGui::SetCursorPos(ImVec2(pos.x, pos.y - 10));
		ImGui::TextEx(TOSTDSTRING(inval).c_str());
        
		//how much data we store
		persist int prevstoresize = 100;
		persist int storesize = 100;
        
		//how often we update
		persist int fupdate = 1;
		persist int frame_count = 0;
        
		persist float maxval = inval + 5;
		persist float minval = inval - 5;
        
		//if(inval > maxval) maxval = inval;
		//if(inval < minval) minval = inval;
        
		if(inval > maxval || inval < minval){
			maxval = inval + 5;
			minval = inval - 5;
		}
		//real values and printed values
		persist std::vector<float> values(storesize);
		persist std::vector<float> pvalues(storesize);
        
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
			values[values.size() - 1] = inval;
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
        
		ImGui::SetCursorPos(ImGui::vec2ToImVec2(pos));
		ImGui::PlotLines("", &pvalues[0], pvalues.size(), 0, 0, minval, maxval, ImVec2(sizex, sizey));
        
		ImGui::PopStyleColor();
		ImGui::PopStyleColor();
	}
    
	void AddPadding(float x){
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + x);
	}
    
} //namespace ImGui


void MenuBar(){
	ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 0);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
	ImGui::PushStyleColor(ImGuiCol_PopupBg, ImGui::ColorToImVec4(color(20, 20, 20, 255)));
	ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImGui::ColorToImVec4(color(20, 20, 20, 255)));
    
	if(ImGui::BeginMainMenuBar()){
		WinHovCheck;
		menubarheight = ImGui::GetWindowHeight();
        
		//// level menu options ////
		if(ImGui::BeginMenu("Level")){
			WinHovCheck;
			if(ImGui::MenuItem("New")){
				AtmoAdmin->Reset();
			}
			if(ImGui::MenuItem("Save")){
				if(level_name == ""){
					ERROR("Level not saved before; Use 'Save As'");
				}
				else{
					//AtmoAdmin->SaveTEXT(level_name);
				}
			}
			if(ImGui::BeginMenu("Save As")){
				WinHovCheck;
				persist char buff[255] = {};
				if(ImGui::InputText("##level_saveas_input", buff, 255, ImGuiInputTextFlags_EnterReturnsTrue)){
					//AtmoAdmin->SaveTEXT(buff);
					level_name = buff;
				}
				ImGui::EndMenu();
			}
			if(ImGui::BeginMenu("Load")){
				WinHovCheck;
				dir_levels = Assets::iterateDirectory(Assets::dirLevels());
				forX(di, dir_levels.count){
					if(ImGui::MenuItem(dir_levels[di].str)){
						//AtmoAdmin->LoadTEXT(dir_levels[di]);
						level_name = dir_levels[di];
					}
				}
				ImGui::EndMenu();
			}
			ImGui::EndMenu();
		}
        
		//// load menu options ////
		if(ImGui::BeginMenu("Load")){
			WinHovCheck;
			if(ImGui::BeginMenu("Meshes")){
				WinHovCheck;
				dir_meshes = Assets::iterateDirectory(Assets::dirModels(), ".mesh");
				forX(di, dir_meshes.count){
					bool loaded = false;
					forX(li, Storage::MeshCount()){
						if(strncmp(Storage::MeshName(li), dir_meshes[di].str, dir_meshes[di].size - 5) == 0){
							loaded = true;  break;
						}
					}
					if(!loaded && ImGui::MenuItem(dir_meshes[di].str)){
						WinHovCheck;
						u32 id = Storage::CreateMeshFromFile(dir_meshes[di].str).first;
						if(id) saved_meshes.add(true);
					}
				}
				ImGui::EndMenu();
			}
            
			if(ImGui::BeginMenu("Textures")){
				WinHovCheck;
				dir_textures = Assets::iterateDirectory(Assets::dirTextures());
				forX(di, dir_textures.count){
					bool loaded = false;
					forX(li, Storage::TextureCount()){
						if(strcmp(Storage::TextureName(li), dir_textures[di].str) == 0){
							loaded = true;  break;
						}
					}
					if(!loaded && ImGui::MenuItem(dir_textures[di].str)){
						WinHovCheck;
						Storage::CreateTextureFromFile(dir_textures[di].str);
					}
				}
				ImGui::EndMenu();
			}
            
			if(ImGui::BeginMenu("Materials")){
				WinHovCheck;
				dir_materials = Assets::iterateDirectory(Assets::dirModels(), ".mat");
				forX(di, dir_materials.count){
					bool loaded = false;
					forX(li, Storage::MaterialCount()){
						if(strncmp(Storage::MaterialName(li), dir_materials[di].str, dir_materials[di].size - 4) == 0){
							loaded = true;  break;
						}
					}
					if(!loaded && ImGui::MenuItem(dir_materials[di].str)){
						WinHovCheck;
						u32 id = Storage::CreateMaterialFromFile(dir_materials[di].str).first;
						if(id) saved_materials.add(true);
					}
				}
				ImGui::EndMenu();
			}
            
			if(ImGui::BeginMenu("Models")){
				WinHovCheck;
				dir_models = Assets::iterateDirectory(Assets::dirModels(), ".model");
				forX(di, dir_models.count){
					bool loaded = false;
					forX(li, Storage::ModelCount()){
						if(strncmp(Storage::ModelName(li), dir_models[di].str, dir_models[di].size - 6) == 0){
							loaded = true;  break;
						}
					}
					if(!loaded && ImGui::MenuItem(dir_models[di].str)){
						WinHovCheck;
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
            
			if(ImGui::BeginMenu("OBJs")){
				WinHovCheck;
				dir_objs = Assets::iterateDirectory(Assets::dirModels(), ".obj");
				forX(di, dir_objs.count){
					u32 loaded_idx = -1;
					forX(li, Storage::ModelCount()){
						if(strcmp(Storage::ModelName(li), dir_objs[di].str) == 0){
							loaded_idx = li;  break;
						}
					}
					if(ImGui::MenuItem(dir_objs[di].str)){
						WinHovCheck;
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
		if(ImGui::BeginMenu("Window")){
			WinHovCheck;
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
		if(ImGui::BeginMenu("State")){
			WinHovCheck;
			ImGui::Text("state not reimpl yet");
			//if(ImGui::MenuItem("Play"))   AtmoAdmin->ChangeState(GameState_Play);
			//if(ImGui::MenuItem("Debug"))  AtmoAdmin->ChangeState(GameState_Debug);
			//if(ImGui::MenuItem("Editor")) AtmoAdmin->ChangeState(GameState_Editor);
			//if(ImGui::MenuItem("Menu"))   AtmoAdmin->ChangeState(GameState_Menu);
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}
    
	ImGui::PopStyleColor(2);
	ImGui::PopStyleVar(2);
}

void EntitiesTab(){
	persist bool rename_ent = false;
	persist char rename_buffer[DESHI_NAME_SIZE] = {};
	persist Entity* events_ent = 0;
    
	//// selected entity keybinds ////
	//start renaming first selected entity
	//TODO(delle) repair this to work with array
	if(selected_entities.count && DeshInput->KeyPressedAnyMod(Key::F2)){
		rename_ent = true;
		DeshConsole->IMGUI_KEY_CAPTURE = true;
		//if(selected_entities.size > 1) selected_entities.remove(selected_entities.end());
		selected_entities[0]->name = string(rename_buffer);
	}
	//submit renaming entity
	if(rename_ent && DeshInput->KeyPressedAnyMod(Key::ENTER)){
		rename_ent = false;
		DeshConsole->IMGUI_KEY_CAPTURE = false;
		selected_entities[0]->name = string(rename_buffer);
	}
	//stop renaming entity
	if(rename_ent && DeshInput->KeyPressedAnyMod(Key::ESCAPE)){
		rename_ent = false;
		DeshConsole->IMGUI_KEY_CAPTURE = false;
	}
	//delete selected entities
	if(selected_entities.count && DeshInput->KeyPressedAnyMod(Key::DELETE)){
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
			if(ImGui::BeginTable("##entity_list_table", 5, ImGuiTableFlags_BordersInner)){
				ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, fontw * 3.5f);  //visible ImGui::Button
				ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, fontw * 5.f);  //id
				ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch);             //name
				//ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, fontw * 3.5f); //evenbutton
				ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, fontw);        //delete button
                
				forX(ent_idx, AtmoAdmin->entities.count){
					Entity* ent = AtmoAdmin->entities[ent_idx];
					if(!ent) assert(!"NULL entity when creating entity list table");
					ImGui::PushID(ent->id);
					ImGui::TableNextRow();
                    
					//// visible button ////
					ImGui::TableSetColumnIndex(0);
					if(ModelInstance* m = ent->GetAttribute<ModelInstance>()){
						if(ImGui::Button((m->visible) ? "(M)" : "( )", ImVec2(-FLT_MIN, 0.0f))){
							m->ToggleVisibility();
						}
					}
					//else if(Light* l = ent->GetAttribute<Light>()){
					//	if(ImGui::Button((l->active) ? "(L)" : "( )", ImVec2(-FLT_MIN, 0.0f))){
					//		l->active = !l->active;
					//	}
					//}
					else{
						if(ImGui::Button("(?)", ImVec2(-FLT_MIN, 0.0f))){}
					}
                    
					//// id + label (selectable row) ////
					ImGui::TableSetColumnIndex(1);
					char label[8];
					sprintf(label, " %04d ", ent->id);
					u32 selected_idx = -1;
					forI(selected_entities.count){ if(ent == selected_entities[i]){ selected_idx = i; break; } }
					bool is_selected = selected_idx != -1;
					if(ImGui::Selectable(label, is_selected, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap)){
						if(is_selected){
							if(DeshInput->LCtrlDown()){
								selected_entities.remove(selected_idx);
							}
							else{
								selected_entities.clear();
								selected_entities.add(ent);
							}
						}
						else{
							if(DeshInput->LCtrlDown()){
								selected_entities.add(ent);
							}
							else{
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
					}
					else{
						ImGui::TextEx(ent->name.str);
					}
                    
					//// events button ////
					//ImGui::TableSetColumnIndex(3);
					//if(ImGui::Button("Events", ImVec2(-FLT_MIN, 0.0f))){
					//	events_ent = (events_ent != ent) ? ent : 0;
					//}
					//EventsMenu(events_ent);
                    
					//// delete button ////
					ImGui::TableSetColumnIndex(4);
					if(ImGui::Button("X", ImVec2(-FLT_MIN, 0.0f))){
						if(is_selected) selected_entities.remove(selected_idx);
						//AtmoAdmin->DeleteEntity(ent);
					}
					ImGui::PopID();
				}
				ImGui::EndTable();
			}
		}
		ImGui::EndChild();
	}//Entity List Scroll child window
	ImGui::PopStyleColor();
    
	ImGui::Separator();
    
	//// create new entity ////
	persist const char* presets[] = { "Empty", "StaticMesh" };
	persist int current_preset = 0;
    
	ImGui::SetCursorPosX(ImGui::GetWindowWidth() * 0.025);
	if(ImGui::Button("New Entity")){
		//Entity* ent = 0;
		//std::string ent_name = TOSTDSTRING(presets[current_preset], AtmoAdmin->entities.size);
		//switch (current_preset){
		//case(0):default: { //Empty
		//	ent = AtmoAdmin->CreateEntityNow({}, ent_name.str);
		//}break;
		//case(1): { //Static Mesh
		//	ModelInstance* mc = new ModelInstance();
		//	Physics* phys = new Physics();
		//	phys->staticPosition = true;
		//	Collider* coll = new AABBCollider(vec3{ .5f, .5f, .5f }, phys->mass);
        
		//	ent = AtmoAdmin->CreateEntityNow({ mc, phys, coll }, ent_name.str);
		//}break;
		//}
        
		//selected_entities.clear();
		//if(ent) selected_entities.push_back(ent);
	}
	ImGui::SameLine(); ImGui::Combo("##preset_combo", &current_preset, presets, ArrayCount(presets));
    
	ImGui::Separator();
    
	//// selected entity inspector panel ////
	Entity* sel = selected_entities.count ? selected_entities[0] : 0;
	if(!sel) return;
	ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 5.0f);
	ImGui::SetCursorPosX(ImGui::GetWindowWidth() * 0.025);
	if(ImGui::BeginChild("##ent_inspector", ImVec2(ImGui::GetWindowWidth() * 0.95f, ImGui::GetWindowHeight() * .9f), true, ImGuiWindowFlags_NoScrollbar)){
        
		//// name ////
		SetPadding; ImGui::TextEx(TOSTDSTRING(sel->id, ":").c_str());
		ImGui::SameLine(); ImGui::SetNextItemWidth(-FLT_MIN); ImGui::InputText("##ent_name_input", sel->name.str, DESHI_NAME_SIZE,
                                                                               ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);
        
		//// transform ////
		int tree_flags = ImGuiTreeNodeFlags_DefaultOpen;
		if(ImGui::CollapsingHeader("Transform", 0, tree_flags)){
		    ImGui::Indent();
			vec3 oldVec = sel->transform.position;
            
			ImGui::TextEx("Position    "); ImGui::SameLine();
			if(ImGui::Inputvec3("##ent_pos", &sel->transform.position)){
				//if(Physics* p = sel->GetAttribute<Physics>()){
				//	p->position = sel->transform.position;
				//	admin->editor.undo_manager.AddUndoTranslate(&sel->transform, &oldVec, &p->position);
				//}
				//else{
				//	admin->editor.undo_manager.AddUndoTranslate(&sel->transform, &oldVec, &sel->transform.position);
				//}
			}ImGui::Separator();
            
			oldVec = sel->transform.rotation;
			ImGui::TextEx("Rotation    "); ImGui::SameLine();
			if(ImGui::Inputvec3("##ent_rot", &sel->transform.rotation)){
				//if(Physics* p = sel->GetAttribute<Physics>()){
				//	p->rotation = sel->transform.rotation;
				//	admin->editor.undo_manager.AddUndoRotate(&sel->transform, &oldVec, &p->rotation);
				//}
				//else{
				//	admin->editor.undo_manager.AddUndoRotate(&sel->transform, &oldVec, &sel->transform.rotation);
				//}
			}ImGui::Separator();
            
			oldVec = sel->transform.scale;
			ImGui::TextEx("Scale       "); ImGui::SameLine();
			if(ImGui::Inputvec3("##ent_scale", &sel->transform.scale)){
				//if(Physics* p = sel->GetAttribute<Physics>()){
				//	p->scale = sel->transform.scale;
				//	admin->editor.undo_manager.AddUndoScale(&sel->transform, &oldVec, &p->scale);
				//}
				//else{
				//	admin->editor.undo_manager.AddUndoScale(&sel->transform, &oldVec, &sel->transform.scale);
				//}
			}ImGui::Separator();
			ImGui::Unindent();
		}
        
		//// components ////
		std::vector<Attribute*> comp_deleted_queue;
		forX(comp_idx, sel->attributes.count){
			Attribute* c = sel->attributes[comp_idx];
			bool delete_button = true;
			ImGui::PushID(c);
            
			switch (c->type){
				//mesh
				case AttributeType_ModelInstance: {
					if(ImGui::CollapsingHeader("Model", &delete_button, tree_flags)){
						ImGui::Indent();
						ModelInstance* mc = dyncast(ModelInstance, c);
                        
						ImGui::TextEx("Visible  "); ImGui::SameLine();
						if(ImGui::Button((mc->visible) ? "True" : "False", ImVec2(-FLT_MIN, 0))){
							mc->ToggleVisibility();
						}
                        
						ImGui::TextEx("Model     "); ImGui::SameLine(); ImGui::SetNextItemWidth(-1);
						if(ImGui::BeginCombo("##model_combo", mc->model->name)){
							forI(Storage::ModelCount()){
								if(ImGui::Selectable(Storage::ModelName(i), mc->model == Storage::ModelAt(i))){
									mc->ChangeModel(Storage::ModelAt(i));
								}
							}
							ImGui::EndCombo();
						}
                        
						ImGui::Unindent();
						ImGui::Separator();
					}
				}break;
                
                //		//physics
                //	case AttributeType_Physics:
                //		if(ImGui::CollapsingHeader("Physics", &delete_button, tree_flags)){
                //			ImGui::Indent();
                
                //			Physics* d = dyncast(Physics, c);
                //			ImGui::TextEx("Velocity     "); ImGui::SameLine(); ImGui::Inputvec3("##phys_vel", &d->velocity);
                //			ImGui::TextEx("Accelertaion "); ImGui::SameLine(); ImGui::Inputvec3("##phys_accel", &d->acceleration);
                //			ImGui::TextEx("Rot Velocity "); ImGui::SameLine(); ImGui::Inputvec3("##phys_rotvel", &d->rotVelocity);
                //			ImGui::TextEx("Rot Accel    "); ImGui::SameLine(); ImGui::Inputvec3("##phys_rotaccel", &d->rotAcceleration);
                //			ImGui::TextEx("Elasticity   "); ImGui::SameLine();
                //			ImGui::SetNextItemWidth(-FLT_MIN); ImGui::InputFloat("##phys_elastic", &d->elasticity);
                //			ImGui::TextEx("Mass         "); ImGui::SameLine();
                //			ImGui::SetNextItemWidth(-FLT_MIN); ImGui::InputFloat("##phys_mass", &d->mass);
                //			ImGui::TextEx("Kinetic Fric "); ImGui::SameLine();
                //			ImGui::SetNextItemWidth(-FLT_MIN); ImGui::InputFloat("##phys_kinfric", &d->kineticFricCoef);
                //			ImGui::Checkbox("Static Position", &d->staticPosition);
                //			ImGui::Checkbox("Static Rotation", &d->staticRotation);
                //			ImGui::Checkbox("2D Physics", &d->twoDphys);
                
                //			ImGui::Unindent();
                //			ImGui::Separator();
                //		}
                //		break;
                
                //		//colliders
                //	case AttributeType_Collider: {
                //		if(ImGui::CollapsingHeader("Collider", &delete_button, tree_flags)){
                //			ImGui::Indent();
                
                //			Collider* coll = dyncast(Collider, c);
                //			f32 mass = 1.0f;
                
                //			ImGui::TextEx("Shape "); ImGui::SameLine(); ImGui::SetNextItemWidth(-1);
                //			if(ImGui::BeginCombo("##coll_type_combo", ColliderShapeStrings[coll->shape])){
                //				forI(ArrayCount(ColliderShapeStrings)){
                //					if(ImGui::Selectable(ColliderShapeStrings[i], coll->shape == i) && (coll->shape != i)){
                //						if(Physics* p = sel->GetAttribute<Physics>()) mass = p->mass;
                
                //						sel->RemoveAttribute(coll);
                //						coll = 0;
                //						switch (i){
                //						case ColliderShape_AABB: {
                //							coll = new AABBCollider(vec3{ 0.5f, 0.5f, 0.5f }, mass);
                //						}break;
                //						case ColliderShape_Box: {
                //							coll = new BoxCollider(vec3{ 0.5f, 0.5f, 0.5f }, mass);
                //						}break;
                //						case ColliderShape_Sphere: {
                //							coll = new SphereCollider(1.0f, mass);
                //						}break;
                //						case ColliderShape_Landscape: {
                //							//coll = new LandscapeCollider();
                //							WARNING_LOC("Landscape collider not setup yet");
                //						}break;
                //						case ColliderShape_Complex: {
                //							//coll = new ComplexCollider();
                //							WARNING_LOC("Complex collider not setup yet");
                //						}break;
                //						}
                
                //						if(coll){
                //							sel->AddAttribute(coll);
                //							admin->AddAttributeToLayers(coll);
                //						}
                //					}
                //				}
                //				ImGui::EndCombo();
                //			}
                
                //			switch (coll->shape){
                //			case ColliderShape_Box: {
                //				BoxCollider* coll_box = dyncast(BoxCollider, coll);
                //				ImGui::TextEx("Half Dims "); ImGui::SameLine();
                //				if(ImGui::Inputvec3("##coll_halfdims", &coll_box->halfDims)){
                //					if(Physics* p = sel->GetAttribute<Physics>()) mass = p->mass;
                //					coll_box->RecalculateTensor(mass);
                //				}
                //			}break;
                //			case ColliderShape_AABB: {
                //				AABBCollider* coll_aabb = dyncast(AABBCollider, coll);
                //				ImGui::TextEx("Half Dims "); ImGui::SameLine();
                //				if(ImGui::Inputvec3("##coll_halfdims", &coll_aabb->halfDims)){
                //					if(Physics* p = sel->GetAttribute<Physics>()) mass = p->mass;
                //					coll_aabb->RecalculateTensor(mass);
                //				}
                //			}break;
                //			case ColliderShape_Sphere: {
                //				SphereCollider* coll_sphere = dyncast(SphereCollider, coll);
                //				ImGui::TextEx("Radius    "); ImGui::SameLine(); ImGui::SetNextItemWidth(-FLT_MIN);
                //				if(ImGui::InputFloat("##coll_sphere", &coll_sphere->radius)){
                //					if(Physics* p = sel->GetAttribute<Physics>()) mass = p->mass;
                //					coll_sphere->RecalculateTensor(mass);
                //				}
                //			}break;
                //			case ColliderShape_Landscape: {
                //				ImGui::TextEx("Landscape collider has no settings yet");
                //			}break;
                //			case ColliderShape_Complex: {
                //				ImGui::TextEx("Complex collider has no settings yet");
                //			}break;
                //			}
                
                //			ImGui::Checkbox("Don't Resolve Collisions", (bool*)&coll->noCollide);
                //			ImGui::TextEx("Collision Layer"); ImGui::SameLine(); ImGui::SetNextItemWidth(-1);
                //			local u32 min = 0, max = 9;
                //			ImGui::SliderScalar("##coll_layer", ImGuiDataType_U32, &coll->layer, &min, &max, "%d");
                
                //			ImGui::Unindent();
                //			ImGui::Separator();
                //		}
                //	}break;
                
                //		//audio listener
                //	case AttributeType_AudioListener: {
                //		if(ImGui::CollapsingHeader("Audio Listener", &delete_button, tree_flags)){
                //			ImGui::Indent();
                
                //			ImGui::TextEx("TODO implement audio listener component editing");
                
                //			ImGui::Unindent();
                //			ImGui::Separator();
                //		}
                //	}break;
                
                //		//audio source
                //	case AttributeType_AudioSource: {
                //		if(ImGui::CollapsingHeader("Audio Source", &delete_button, tree_flags)){
                //			ImGui::Indent();
                
                //			ImGui::TextEx("TODO implement audio source component editing");
                
                //			ImGui::Unindent();
                //			ImGui::Separator();
                //		}
                //	}break;
                
                //		//camera
                //	case AttributeType_Camera: {
                //		if(ImGui::CollapsingHeader("CameraInstance", &delete_button, tree_flags)){
                //			ImGui::Indent();
                
                //			ImGui::TextEx("TODO implement camera component editing");
                
                //			ImGui::Unindent();
                //			ImGui::Separator();
                //		}
                //	}break;
                
                //		//light
                //	case AttributeType_Light: {
                //		if(ImGui::CollapsingHeader("Light", &delete_button, tree_flags)){
                //			ImGui::Indent();
                
                //			Light* d = dyncast(Light, c);
                //			ImGui::TextEx("Brightness   "); ImGui::SameLine(); ImGui::SetNextItemWidth(-FLT_MIN);
                //			ImGui::InputFloat("##light_brightness", &d->brightness);
                //			ImGui::TextEx("Position     "); ImGui::SameLine();
                //			ImGui::Inputvec3("##light_position", &d->position);
                //			ImGui::TextEx("Direction    "); ImGui::SameLine();
                //			ImGui::Inputvec3("##light_direction", &d->direction);
                
                //			ImGui::Unindent();
                //			ImGui::Separator();
                //		}
                //	}break;
                
                //		//orb manager
                //	case AttributeType_OrbManager: {
                //		if(ImGui::CollapsingHeader("Orbs", &delete_button, tree_flags)){
                //			ImGui::Indent();
                
                //			OrbManager* d = dyncast(OrbManager, c);
                //			ImGui::TextEx("Orb Count "); ImGui::SameLine(); ImGui::SetNextItemWidth(-FLT_MIN);
                //			ImGui::InputInt("##orb_orbcount", &d->orbcount);
                
                //			ImGui::Unindent();
                //			ImGui::Separator();
                //		}
                //	}break;
                
                //		//door
                //	case AttributeType_Door: {
                //		if(ImGui::CollapsingHeader("Door", &delete_button, tree_flags)){
                //			ImGui::Indent();
                
                //			ImGui::TextEx("TODO implement door component editing");
                
                //			ImGui::Unindent();
                //			ImGui::Separator();
                //		}
                //	}break;
                
                //		//player
                //	case AttributeType_Player: {
                //		if(ImGui::CollapsingHeader("Player", &delete_button, tree_flags)){
                //			ImGui::Indent();
                
                //			Player* d = dyncast(Player, c);
                //			ImGui::TextEx("Health "); ImGui::SameLine(); ImGui::SetNextItemWidth(-FLT_MIN);
                //			ImGui::InputInt("##player_health", &d->health);
                
                //			ImGui::Unindent();
                //			ImGui::Separator();
                //		}
                //	}break;
                
                //		//movement
                //	case AttributeType_Movement: {
                //		if(ImGui::CollapsingHeader("Movement", &delete_button, tree_flags)){
                //			ImGui::Indent();
                
                //			Movement* d = dyncast(Movement, c);
                //			ImGui::TextEx("Ground Accel    "); ImGui::SameLine(); ImGui::SetNextItemWidth(-FLT_MIN);
                //			ImGui::InputFloat("##move_gndaccel", &d->gndAccel);
                //			ImGui::TextEx("Air Accel       "); ImGui::SameLine(); ImGui::SetNextItemWidth(-FLT_MIN);
                //			ImGui::InputFloat("##move_airaccel", &d->airAccel);
                //			ImGui::TextEx("Jump Impulse    "); ImGui::SameLine(); ImGui::SetNextItemWidth(-FLT_MIN);
                //			ImGui::InputFloat("##move_jimp", &d->jumpImpulse);
                //			ImGui::TextEx("Max Walk Speed  "); ImGui::SameLine(); ImGui::SetNextItemWidth(-FLT_MIN);
                //			ImGui::InputFloat("##move_maxwalk", &d->maxWalkingSpeed);
                //			ImGui::TextEx("Max Run Speed   "); ImGui::SameLine(); ImGui::SetNextItemWidth(-FLT_MIN);
                //			ImGui::InputFloat("##move_maxrun", &d->maxRunningSpeed);
                //			ImGui::TextEx("Max Crouch Speed"); ImGui::SameLine(); ImGui::SetNextItemWidth(-FLT_MIN);
                //			ImGui::InputFloat("##move_maxcrouch", &d->maxCrouchingSpeed);
                
                //			ImGui::Unindent();
                //			ImGui::Separator();
                //		}
                //	}break;
			}
            
			if(!delete_button) comp_deleted_queue.push_back(c);
			ImGui::PopID();
		} //for(Attribute* c : sel->components)
		//sel->RemoveAttributes(comp_deleted_queue);
        
		//// add component ////
		persist int add_comp_index = 0;
        
		ImGui::SetCursorPosX(ImGui::GetWindowWidth() * 0.025);
		if(ImGui::Button("Add Attribute")){
			switch (1 << (add_comp_index - 1)){
				case AttributeType_ModelInstance: {
					Attribute* comp = new ModelInstance();
					sel->attributes.add(comp);
					//admin->AddAttributeToLayers(comp);
				}break;
                //	case AttributeType_Physics: {
                //		Attribute* comp = new Physics();
                //		sel->AddAttribute(comp);
                //		admin->AddAttributeToLayers(comp);
                //	}break;
                //	case AttributeType_Collider: {
                //		Attribute* comp = new AABBCollider(vec3{ .5f, .5f, .5f }, 1.f);
                //		sel->AddAttribute(comp);
                //		admin->AddAttributeToLayers(comp);
                //	}break;
                //	case AttributeType_AudioListener: {
                //		Attribute* comp = new AudioListener(sel->transform.position, vec3::ZERO, sel->transform.rotation);
                //		sel->AddAttribute(comp);
                //		admin->AddAttributeToLayers(comp);
                //	}break;
                //	case AttributeType_AudioSource: { //!Incomplete
                //		Attribute* comp = new AudioSource();
                //		sel->AddAttribute(comp);
                //		admin->AddAttributeToLayers(comp);
                //	}break;
                //	case AttributeType_Camera: {
                //		Attribute* comp = new CameraInstance(90.f);
                //		sel->AddAttribute(comp);
                //		admin->AddAttributeToLayers(comp);
                //	}break;
                //	case AttributeType_Light: {
                //		Attribute* comp = new Light(sel->transform.position, sel->transform.rotation);
                //		sel->AddAttribute(comp);
                //		admin->AddAttributeToLayers(comp);
                //	}break;
                //	case AttributeType_OrbManager: { //!Incomplete
                //		Attribute* comp = new OrbManager(0);
                //		sel->AddAttribute(comp);
                //		admin->AddAttributeToLayers(comp);
                //	}break;
                //	case AttributeType_Door: {
                //		Attribute* comp = new Door();
                //		sel->AddAttribute(comp);
                //		admin->AddAttributeToLayers(comp);
                //	}break;
                //	case AttributeType_Player: { //!Incomplete
                //		Attribute* comp = new Player();
                //		sel->AddAttribute(comp);
                //		admin->AddAttributeToLayers(comp);
                //	}break;
                //	case AttributeType_Movement: { //!Incomplete
                //		Attribute* comp = new Movement();
                //		sel->AddAttribute(comp);
                //		admin->AddAttributeToLayers(comp);
                //	}break;
                //	case(0):default: { //None
                //		//do nothing
                //	}break;
			}
		}
		ImGui::SameLine(); ImGui::SetNextItemWidth(-1);
		ImGui::Combo("##add_comp_combo", &add_comp_index, AttributeTypeStrings, ArrayCount(AttributeTypeStrings));
        
		ImGui::EndChild(); //CreateMenu
	}
	ImGui::PopStyleVar(); //ImGuiStyleVar_IndentSpacing
} //EntitiesTab

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
	if(selected && DeshInput->KeyPressedAnyMod(Key::F2)){
		rename_mesh = true;
		DeshConsole->IMGUI_KEY_CAPTURE = true;
		cpystr(rename_buffer, selected->name, DESHI_NAME_SIZE);
	}
	//submit renaming mesh
	if(selected && rename_mesh && DeshInput->KeyPressedAnyMod(Key::ENTER)){
		rename_mesh = false;
		DeshConsole->IMGUI_KEY_CAPTURE = false;
		cpystr(selected->name, rename_buffer, DESHI_NAME_SIZE);
	}
	//stop renaming mesh
	if(rename_mesh && DeshInput->KeyPressedAnyMod(Key::ESCAPE)){
		rename_mesh = false;
		DeshConsole->IMGUI_KEY_CAPTURE = false;
	}
	//delete mesh
	if(selected && DeshInput->KeyPressedAnyMod(Key::DELETE)){
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
	persist color text_color = color::WHITE;
	color vertex_color = color::GREEN; //non-static so they can be changed
	color triangle_color = color::RED;
	color face_color = color::BLUE;
	persist color selected_color = color(255, 255, 0, 128); //yellow  half-alphs
	persist color neighbor_color = color(255, 0, 255, 128); //megenta half-alpha
	persist color edge_color = color(0, 255, 255, 128); //cyan    half-alpha
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
			vec3 tri_center = Geometry::MeshTriangleMidpoint(sel_triangle);
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
			vec3 tri_center = Geometry::MeshTriangleMidpoint(sel_triangle) * scale;
			Render::DrawTriangleFilled(sel_triangle->p[0] * scale, sel_triangle->p[1] * scale, sel_triangle->p[2] * scale, selected_color);
			if(triangle_indexes) ImGui::DebugDrawText3(TOSTRING("T", sel_triangle_idx).str, tri_center, text_color, vec2{ -5,-5 });
			if(triangle_centers) Render::DrawBoxFilled(mat4::TransformationMatrix(tri_center, vec3::ZERO, vec3{ .05f,.05f,.05f }), selected_color);
			if(triangle_normals) Render::DrawLine(tri_center, tri_center + sel_triangle->normal * normal_scale, selected_color);
			forX(tni, sel_triangle->neighbors.count){
				Mesh::Triangle* tri_nei = &selected->triangles[sel_triangle->neighbors[tni]];
				if(trinei_indexes) ImGui::DebugDrawText3(TOSTRING("TN", tni).str, Geometry::MeshTriangleMidpoint(tri_nei) * scale, text_color, vec2{ 10,10 });
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
				vec3 tri_center = Geometry::MeshTriangleMidpoint(sel_triangle) * scale;
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
				if(face_triangle_indexes) ImGui::DebugDrawText3(TOSTRING("FT", fti).str, Geometry::MeshTriangleMidpoint(ft) * scale, text_color, vec2{ -10,-10 });
			}
			forX(fnti, sel_face->neighborTriangleCount){
				MeshTriangle* ft = &selected->triangles[sel_face->triangleNeighbors[fnti]];
				if(face_tri_neighbors) Render::DrawTriangleFilled(ft->p[0] * scale, ft->p[1] * scale, ft->p[2] * scale, neighbor_color);
				if(face_trinei_indexes) ImGui::DebugDrawText3(TOSTRING("FTN", fnti).str, Geometry::MeshTriangleMidpoint(ft) * scale, text_color, vec2{ 10,10 });
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

void TexturesTab(){
	persist u32 sel_tex_idx = -1;
	Texture* selected = nullptr;
	if(sel_tex_idx < Storage::TextureCount()) selected = Storage::TextureAt(sel_tex_idx);
    
	//// selected material keybinds ////
	//delete material
	if(selected && DeshInput->KeyPressedAnyMod(Key::DELETE)){
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

void MaterialsTab(){
	persist u32  sel_mat_idx = -1;
	persist bool rename_mat = false;
	persist char rename_buffer[DESHI_NAME_SIZE] = {};
	Material* selected = nullptr;
	if(sel_mat_idx < Storage::MaterialCount()) selected = Storage::MaterialAt(sel_mat_idx);
    
	//// selected material keybinds ////
	//start renaming material
	if(selected && DeshInput->KeyPressedAnyMod(Key::F2)){
		rename_mat = true;
		DeshConsole->IMGUI_KEY_CAPTURE = true;
		cpystr(rename_buffer, selected->name, DESHI_NAME_SIZE);
	}
	//submit renaming material
	if(selected && rename_mat && DeshInput->KeyPressedAnyMod(Key::ENTER)){
		rename_mat = false;
		DeshConsole->IMGUI_KEY_CAPTURE = false;
		cpystr(selected->name, rename_buffer, DESHI_NAME_SIZE);
	}
	//stop renaming material
	if(rename_mat && DeshInput->KeyPressedAnyMod(Key::ESCAPE)){
		rename_mat = false;
		DeshConsole->IMGUI_KEY_CAPTURE = false;
	}
	//delete material
	if(selected && DeshInput->KeyPressedAnyMod(Key::DELETE)){
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

void ModelsTab(){
	persist u32  sel_model_idx = -1;
	persist u32  sel_batch_idx = -1;
	persist bool rename_model = false;
	persist char rename_buffer[DESHI_NAME_SIZE] = {};
	Model* selected = nullptr;
	if(sel_model_idx < Storage::ModelCount()) selected = Storage::ModelAt(sel_model_idx);
    
	//// selected model keybinds ////
	//start renaming model
	if(selected && DeshInput->KeyPressedAnyMod(Key::F2)){
		rename_model = true;
		DeshConsole->IMGUI_KEY_CAPTURE = true;
		cpystr(rename_buffer, selected->name, DESHI_NAME_SIZE);
	}
	//submit renaming model
	if(selected && rename_model && DeshInput->KeyPressedAnyMod(Key::ENTER)){
		rename_model = false;
		DeshConsole->IMGUI_KEY_CAPTURE = false;
		cpystr(selected->name, rename_buffer, DESHI_NAME_SIZE);
	}
	//stop renaming model
	if(rename_model && DeshInput->KeyPressedAnyMod(Key::ESCAPE)){
		rename_model = false;
		DeshConsole->IMGUI_KEY_CAPTURE = false;
	}
	//delete model
	if(selected && DeshInput->KeyPressedAnyMod(Key::DELETE)){
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


void FontsTab(){
	persist u32 sel_font_idx = -1;
	Font* selected = nullptr;
	//!Incomplete
	ImGui::TextEx("TODO    Move fonts to storage");
} //FontsTab

enum TwodPresets : u32 {
	Twod_NONE = 0, Twod_Line, Twod_Triangle, Twod_Square, Twod_NGon, Twod_Image,
};

void SettingsTab(){
	SetPadding;
	if(ImGui::BeginChild("##settings_tab", ImVec2(ImGui::GetWindowWidth() * 0.95f, ImGui::GetWindowHeight() * .9f))){
		//// physics properties ////
		if(ImGui::CollapsingHeader("Physics", 0)){
			ImGui::TextEx("Pause Physics "); ImGui::SameLine();
			if(ImGui::Button((AtmoAdmin->pause_phys) ? "True" : "False", ImVec2(-FLT_MIN, 0))){
				AtmoAdmin->pause_phys = !AtmoAdmin->pause_phys;
			}
			//ImGui::TextEx("Gravity       "); ImGui::SameLine(); ImGui::InputFloat("##phys_gravity", &AtmoAdmin->physics.gravity);
            
			//ImGui::TextEx("Phys TPS      "); ImGui::SameLine(); ImGui::InputFloat("##phys_tps", );
            
			ImGui::Separator();
		}
        
		//// camera properties ////
		if(ImGui::CollapsingHeader("Camera", 0)){
			if(ImGui::Button("Zero", ImVec2(ImGui::GetWindowWidth() * .45f, 0))){
				editor_camera->position = vec3::ZERO; editor_camera->rotation = vec3::ZERO;
			} ImGui::SameLine();
			if(ImGui::Button("Reset", ImVec2(ImGui::GetWindowWidth() * .45f, 0))){
				editor_camera->position = { 4.f,3.f,-4.f }; editor_camera->rotation = { 28.f,-45.f,0.f };
			}
            
			ImGui::TextEx("Position  "); ImGui::SameLine(); ImGui::Inputvec3("##cam_pos", &editor_camera->position);
			ImGui::TextEx("Rotation  "); ImGui::SameLine(); ImGui::Inputvec3("##cam_rot", &editor_camera->rotation);
			ImGui::TextEx("Near Clip "); ImGui::SameLine();
			if(ImGui::InputFloat("##cam_nearz", &editor_camera->nearZ)){
				editor_camera->UpdateProjectionMatrix();
			}
			ImGui::TextEx("Far Clip  "); ImGui::SameLine();
			if(ImGui::InputFloat("##cam_farz", &editor_camera->farZ)){
				editor_camera->UpdateProjectionMatrix();
			};
			ImGui::TextEx("FOV       "); ImGui::SameLine();
			if(ImGui::InputFloat("##cam_fov", &editor_camera->fov)){
				editor_camera->UpdateProjectionMatrix();
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
            
			ImGui::Separator();
		}
        
		ImGui::EndChild();
	}
}//settings tab

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
    
	ImGui::PushStyleColor(ImGuiCol_Border, ImGui::ColorToImVec4(color(0, 0, 0)));
	ImGui::PushStyleColor(ImGuiCol_Button, ImGui::ColorToImVec4(color(40, 40, 40)));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImGui::ColorToImVec4(color(48, 48, 48)));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::ColorToImVec4(color(60, 60, 60)));
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImColor(0xff141414).Value);
	ImGui::PushStyleColor(ImGuiCol_PopupBg, ImGui::ColorToImVec4(color(20, 20, 20)));
	ImGui::PushStyleColor(ImGuiCol_FrameBg, ImGui::ColorToImVec4(color(35, 45, 50)));
	ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImGui::ColorToImVec4(color(42, 54, 60)));
	ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImGui::ColorToImVec4(color(54, 68, 75)));
	ImGui::PushStyleColor(ImGuiCol_TitleBg, ImGui::ColorToImVec4(color(0, 0, 0)));
	ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImGui::ColorToImVec4(color(0, 0, 0)));
	ImGui::PushStyleColor(ImGuiCol_Header, ImGui::ColorToImVec4(color(35, 45, 50)));
	ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImGui::ColorToImVec4(color(0, 74, 74)));
	ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImGui::ColorToImVec4(color(0, 93, 93)));
	ImGui::PushStyleColor(ImGuiCol_TableBorderLight, ImGui::ColorToImVec4(color(45, 45, 45)));
	ImGui::PushStyleColor(ImGuiCol_TableHeaderBg, ImGui::ColorToImVec4(color(10, 10, 10)));
	ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, ImGui::ColorToImVec4(color(10, 10, 10)));
	ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, ImGui::ColorToImVec4(color(55, 55, 55)));
	ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive, ImGui::ColorToImVec4(color(75, 75, 75)));
	ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, ImGui::ColorToImVec4(color(65, 65, 65)));
	ImGui::PushStyleColor(ImGuiCol_TabActive, ImGui::ColorToImVec4(color::VERY_DARK_CYAN));
	ImGui::PushStyleColor(ImGuiCol_TabHovered, ImGui::ColorToImVec4(color::DARK_CYAN));
	ImGui::PushStyleColor(ImGuiCol_Tab, ImColor(0xff0d2b45).Value);
	ImGui::PushStyleColor(ImGuiCol_Separator, ImGui::ColorToImVec4(color::VERY_DARK_CYAN));
    
	ImGuiWindowFlags window_flags;
	if(popoutInspector){
		window_flags = ImGuiWindowFlags_None;
	}else{
		//resize tool menu if main menu bar is open
		ImGui::SetNextWindowSize(ImVec2(DeshWindow->width / 5, DeshWindow->height - (menubarheight + debugbarheight)));
		ImGui::SetNextWindowPos(ImVec2(0, menubarheight));
		window_flags = ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove
			| ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar
			| ImGuiWindowFlags_NoScrollWithMouse;
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
				if(ImGui::BeginTabItem("Meshes"))    { MeshesTab();    ImGui::EndTabItem(); }
				if(ImGui::BeginTabItem("Textures"))  { TexturesTab();  ImGui::EndTabItem(); }
				if(ImGui::BeginTabItem("Materials")){ MaterialsTab(); ImGui::EndTabItem(); }
				if(ImGui::BeginTabItem("Models"))    { ModelsTab();    ImGui::EndTabItem(); }
				if(ImGui::BeginTabItem("Fonts"))     { /*FontsTab();*/ ImGui::EndTabItem(); }
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
    ImGui::PushStyleColor(ImGuiCol_Border, ImGui::ColorToImVec4(color(0, 0, 0, 255)));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImGui::ColorToImVec4(color(20, 20, 20, 255)));
    ImGui::PushStyleColor(ImGuiCol_TableBorderLight, ImGui::ColorToImVec4(color(45, 45, 45, 255)));
    ImGui::SetNextWindowSize(ImVec2(DeshWindow->width, 20));
    ImGui::SetNextWindowPos(ImVec2(0, DeshWindow->height - 20));
    ImGui::Begin("DebugBar", (bool*)1, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);
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
                float strlen6 = (fontw) * str6.size;
                ImGui::SameLine((ImGui::GetColumnWidth() - strlen6) / 2); ImGui::PushItemWidth(-1);
                ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorToImVec4(color(col_text)));
                ImGui::TextEx(str6.str);
                ImGui::PopStyleColor();
            }
        }
        
        //Show Time
        if(ImGui::TableNextColumn()){
            string str7 = DeshTime->FormatDateTime("{h}:{m}:{s}").c_str();
            float strlen7 = fontw * str7.size;
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


//void DisplayTriggers(Admin* admin){
//	int i = 0;
//	for(Entity* e : AtmoAdmin->entities){
//		if(e->type == EntityType_Trigger){
//			Trigger* t = dyncast(Trigger, e);
//			switch (t->collider->shape){
//			case ColliderShape_AABB: {
//				Render::DrawBox(e->transform.TransformMatrix(), color::DARK_MAGENTA);
//			}break;
//			case ColliderShape_Sphere: {
//
//			}break;
//			case ColliderShape_Complex: {
//
//			}break;
//			}
//		}
//		i++;
//	}
//}

void WorldGrid(){
	int lines = 100;
	f32 xp = floor(editor_camera->position.x) + lines;
	f32 xn = floor(editor_camera->position.x) - lines;
	f32 zp = floor(editor_camera->position.z) + lines;
	f32 zn = floor(editor_camera->position.z) - lines;
    
	color color(50, 50, 50);
	for(int i = 0; i < lines * 2 + 1; i++){
		vec3 v1 = vec3(xn + i, 0, zn);
		vec3 v2 = vec3(xn + i, 0, zp);
		vec3 v3 = vec3(xn, 0, zn + i);
		vec3 v4 = vec3(xp, 0, zn + i);
        
		if(xn + i != 0) Render::DrawLine(v1, v2, color);
		if(zn + i != 0) Render::DrawLine(v3, v4, color);
	}
    
	Render::DrawLine(vec3{ -1000,0,0 }, vec3{ 1000,0,0 }, color::RED);
	Render::DrawLine(vec3{ 0,-1000,0 }, vec3{ 0,1000,0 }, color::GREEN);
	Render::DrawLine(vec3{ 0,0,-1000 }, vec3{ 0,0,1000 }, color::BLUE);
}

void ShowWorldAxis(){
	vec3
        x = editor_camera->position + editor_camera->forward + vec3::RIGHT * 0.1,
	y = editor_camera->position + editor_camera->forward + vec3::UP * 0.1,
	z = editor_camera->position + editor_camera->forward + vec3::FORWARD * 0.1;
    
	vec2
        spx = Math::WorldToScreen2(x, editor_camera->projMat, editor_camera->viewMat, DeshWindow->dimensions) - DeshWindow->dimensions / 2,
	spy = Math::WorldToScreen2(y, editor_camera->projMat, editor_camera->viewMat, DeshWindow->dimensions) - DeshWindow->dimensions / 2,
	spz = Math::WorldToScreen2(z, editor_camera->projMat, editor_camera->viewMat, DeshWindow->dimensions) - DeshWindow->dimensions / 2;
    
	vec2 offset = vec2(DeshWindow->width - 50, DeshWindow->height - debugbarheight - 50);
    
	Render::DrawLineUI(offset, spx + offset, 1, color::RED);
	Render::DrawLineUI(offset, spy + offset, 1, color::GREEN);
	Render::DrawLineUI(offset, spz + offset, 1, color::BLUE);
}

void Editor::Init(){
	selected_entities.reserve(8);
	level_name = "";
    editor_camera = &AtmoAdmin->camera;
    
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

void Editor::Update(){
	fonth = ImGui::GetFontSize();
	fontw = fonth / 2.f;
	
	{//general
		if(DeshInput->KeyPressed(Key::P | InputMod_Lctrl))
			AtmoAdmin->paused = !AtmoAdmin->paused;
	}
    
	{//select
        
	}
    
	{//render
		if(DeshInput->KeyPressed(Key::F5)) Render::ReloadAllShaders();
        
		//fullscreen toggle
		if(DeshInput->KeyPressed(Key::F11)){
			if(DeshWindow->displayMode == DisplayMode_Windowed || DeshWindow->displayMode == DisplayMode_Borderless)
				DeshWindow->UpdateDisplayMode(DisplayMode_Fullscreen);
			else 
				DeshWindow->UpdateDisplayMode(DisplayMode_Windowed);
		}
	}
    
	{//camera
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
			editor_camera->position = selected_entities[0]->transform.position + vec3(4.f, 3.f, -4.f);
			editor_camera->rotation = { 28.f, -45.f, 0.f };
		}
	}
    
    
	{//interface 
		if(DeshInput->KeyPressed(AtmoAdmin->controller.toggleConsole))   DeshConsole->dispcon = !DeshConsole->dispcon;
		if(DeshInput->KeyPressed(AtmoAdmin->controller.toggleDebugMenu)) showInspector = !showInspector;
		if(DeshInput->KeyPressed(AtmoAdmin->controller.toggleDebugBar))  showDebugBar = !showDebugBar;
		if(DeshInput->KeyPressed(AtmoAdmin->controller.toggleMenuBar))   showMenuBar = !showMenuBar;
	}
    
	{//cut/copy/paste
		//TODO(sushi) reimpl cut/copy/paste
	}
    
    
	///////////////////////////////
	//// render user interface ////
	///////////////////////////////
    
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

void Editor::Reset(){
	selected_entities.clear();
	undos.clear();
	redos.clear();
    
}

void Editor::Cleanup(){
    
}