/* Bookmarks:
@init @cleanup @update
@input        keyboard and mouse handling
@gizmo_input  transform gizmo input handling
@gizmo_draw   transform gizmo drawing
@grid         world grid drawing
@ui_minimized minimized editor
@ui_expanded  normal editor
@scene_ui     entity list and properties
@undos_ui     undos/redos list
@config_ui    editor config
@leveldir_ui popup for saving/loading a level
@ui_other     other window drawing (metrics, storage)
*/

#pragma once
#ifndef ATMOS_EDITOR2_H
#define ATMOS_EDITOR2_H

void InitEditor();
void UpdateEditor();
void CleanupEditor();

#endif //ATMOS_EDITOR2_H
#ifdef ATMOS_IMPLEMENTATION

struct EditorConfig{    //defaults
	cstring font_name;    //gohufont-11.bdf
	f32 font_height;      //11
	
	b32 editor_minimized; //false
	b32 editor_pinned;    //false
	vec2 editor_pos;      //{0, 0}
	vec2 editor_size;     //{window width / 5, window height}
	
	b32 draw_grid;        //true
	b32 draw_metrics;     //false
	b32 draw_storage;     //false
	b32 draw_ui_demo;     //false
};

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

enum EditorMenuBarTab{
	EditorMenuBarTab_NONE,
	EditorMenuBarTab_Level, //new, save, save as, load
	EditorMenuBarTab_View,  //graph, metrics, storage
	EditorMenuBarTab_State,
};

Font* editor_font = 0;
Type  editor_transform_type = TransformType_NONE;
Type  editor_menu_bar_tab = EditorMenuBarTab_NONE;
b32   editor_show_leveldir = false;
b32   editor_leveldir_saving = false;
char  editor_leveldir_dir_buffer[256] = {};
char  editor_leveldir_file_buffer[256] = {};

EditorConfig editor_config;
array<Entity*> editor_selected_entities;

///////////////
//// @init ////
///////////////
void
InitEditor(){
	editor_config.font_name   = cstr_lit("gohufont-11.bdf");
	editor_config.font_height = 11;
	editor_config.editor_minimized = false;
	editor_config.editor_pinned    = false;
	editor_config.editor_pos       = vec2::ZERO;
	editor_config.editor_size      = vec2{f32(DeshWindow->width)/5.f, f32(DeshWindow->height)};
	editor_config.draw_grid    = true;
	editor_config.draw_metrics = false;
	editor_config.draw_storage = false;
	editor_config.draw_ui_demo = false;
	//TODO load editor config file
	
	editor_font = Storage::CreateFontFromFile(editor_config.font_name, editor_config.font_height).second;
	cstring levels_dir = absolute_path("data/levels/");
	CopyMemory(editor_leveldir_dir_buffer, levels_dir.str, levels_dir.count);
	editor_selected_entities = array<Entity*>(deshi_allocator);
}


//////////////////
//// @cleanup ////
//////////////////
void
CleanupEditor(){
	//TODO save editor config file
}


/////////////////
//// @update ////
/////////////////
#define number_inputtext_flags UIInputTextFlags_Numerical | UIInputTextFlags_EnterReturnsTrue

#define StartDisabled() \
UI::PushColor(UIStyleCol_ButtonBg,        Color_Black); \
UI::PushColor(UIStyleCol_ButtonBgActive,  Color_Black); \
UI::PushColor(UIStyleCol_ButtonBgHovered, Color_Black); \
UI::PushColor(UIStyleCol_Text,            Color_DarkGrey); \

#define StopDisabled() \
UI::PopColor(4);

#define BoolButton(var) \
if(UI::Button((var) ? "True" : "False")) ToggleBool(var)

b32 InputFloat(const char* label, f32* value, const char* fmt = "%g", f32 width = MAX_F32){
	char buffer[32]; snprintf(buffer, ArrayCount(buffer), fmt, *value);
	UI::SetNextItemWidth(width);
	if(UI::InputText(label, buffer, ArrayCount(buffer), 0, number_inputtext_flags)){
		*value = stod(buffer);
		return true;
	}
	return false;
}

b32 InputVec3(const char* label, vec3* value, const char* fmt = "%g", f32 width = MAX_F32){
	width = (width == MAX_F32) ? (UI::GetMarginedRight() - UI::GetWinCursor().x - UI::GetRightIndent()) / 3.f : width / 3.f;
	b32 value_changed = false;
	UI::BeginRow(label, 3, UI::GetStyle().fontHeight * UI::GetStyle().inputTextHeightRelToFont);{ //TODO auto divide size when possible
		UI::RowSetupRelativeColumnWidths({1,1,1});
		value_changed |= InputFloat(label+1, &value->x, fmt, width); //HACK label+x is for unique labels since UI has no way to do that
		value_changed |= InputFloat(label+2, &value->y, fmt, width);
		value_changed |= InputFloat(label+3, &value->z, fmt, width);
	}UI::EndRow();
	return value_changed;
}

void TextCentered(const char* text){
	UI::BeginRow(text, 1, UI::GetStyle().fontHeight);
	UI::RowSetupColumnAlignments({vec2{.5f,.5f}});
	UI::Text(text);
	UI::EndRow();
}

void
UpdateEditor(){
	if(DeshWindow->minimized) return; //NOTE nothing here needs to happen if minimized
	
	//// @input ////
	{
		//// @gizmo_input ////
		b32 using_gizmo = false;
		//TODO reimplement the transform gizmo
		
		//// selection ////
		if(DeshInput->KeyPressed(MouseButton::LEFT) && !using_gizmo && !UI::AnyWinHovered()){
			//NOTE adjusting the projection matrix so the nearZ is at least .1, produces bad results if less
			mat4 adjusted_proj = Camera::MakePerspectiveProjectionMatrix(f32(DeshWindow->width), f32(DeshWindow->height), 
																		 AtmoAdmin->camera.fov, AtmoAdmin->camera.farZ, 
																		 Max(.1f, AtmoAdmin->camera.nearZ));
			vec3 direction = (Math::ScreenToWorld(DeshInput->mousePos, adjusted_proj, AtmoAdmin->camera.viewMat, DeshWindow->dimensions) 
							  - AtmoAdmin->camera.position).normalized();
			
			if(Entity* e = AtmoAdmin->EntityRaycast(AtmoAdmin->camera.position, direction, AtmoAdmin->camera.farZ)){
				if(DeshInput->ShiftDown()){      //add entity to selected
					editor_selected_entities.add(e);
				}else if(DeshInput->CtrlDown()){ //remove entity from selected
					forI(editor_selected_entities.count){
						if(editor_selected_entities[i] == e){
							editor_selected_entities.remove_unordered(i);
							break;
						}
					}
				}else{                           //clear selected, add entity to selected
					editor_selected_entities.clear();
					editor_selected_entities.add(e);
				}
			}else{
				editor_selected_entities.clear();
			}
		}
		
		//// camera ////
		if(DeshInput->KeyPressed(AtmoAdmin->controller.gotoSelected) && editor_selected_entities.count){
			vec3 average_position = editor_selected_entities[0]->transform.position;
			for(u32 i = 1; i < editor_selected_entities.count; ++i){
				average_position += editor_selected_entities[i]->transform.position;
			}
			average_position /= editor_selected_entities.count;
			
			AtmoAdmin->camera.position = average_position + vec3(4.f, 3.f, -4.f);
			AtmoAdmin->camera.rotation = { 28.f, -45.f, 0.f };
		}
		
		//// clipboard //// //TODO readd clipboard
		//if(DeshInput->KeyPressed(AtmoAdmin->controller.cut))   CutEntities();
		//if(DeshInput->KeyPressed(AtmoAdmin->controller.copy))  CopyEntities();
		//if(DeshInput->KeyPressed(AtmoAdmin->controller.paste)) PasteEntities();
		
		//// fullscreen ////
		if(DeshInput->KeyPressed(Key::F11)){
			if(DeshWindow->displayMode == DisplayMode_Windowed || DeshWindow->displayMode == DisplayMode_Borderless){
				DeshWindow->UpdateDisplayMode(DisplayMode_Fullscreen);
			}else{ 
				DeshWindow->UpdateDisplayMode(DisplayMode_Windowed);
			}
		}
		
		//// keybinds ////
		if(DeshInput->KeyPressed(Key::P | InputMod_Lctrl)) ToggleBool(AtmoAdmin->simulateInEditor);
		
		//// render ////
		if(DeshInput->KeyPressed(Key::F5)) Render::ReloadAllShaders();
		
		//// save/load level ////
		if(DeshInput->KeyPressed(AtmoAdmin->controller.saveLevel)){
			if(AtmoAdmin->levelName){
				AtmoAdmin->SaveLevel(cstring{AtmoAdmin->levelName.str,(u64)AtmoAdmin->levelName.count});
			}else{
				editor_show_leveldir = true;
				editor_leveldir_saving = true;
			}
		}
		
		//// undo/redo //// //TODO readd undo
		//if(DeshInput->KeyPressed(AtmoAdmin->controller.undo)) Undo();
		//if(DeshInput->KeyPressed(AtmoAdmin->controller.redo)) Redo();
	}
	
	//// @gizmo_draw ////
	{
		//TODO gizmo drawing
	}
	
	//// @grid ////
	if(editor_config.draw_grid){
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
		
		Render::DrawLine(vec3{-1000,0,0}, vec3{1000,0,0}, Color_Red);
		Render::DrawLine(vec3{0,-1000,0}, vec3{0,1000,0}, Color_Green);
		Render::DrawLine(vec3{0,0,-1000}, vec3{0,0,1000}, Color_Blue);
	}
	
	//// @ui_minimized ////
	if(editor_config.editor_minimized){
		//TODO minimized editor (use header)
	}
	
	//// @ui_expanded ////
	if(!editor_config.editor_minimized){
		UI::PushFont(editor_font);
		UI::PushVar(UIStyleVar_FontHeight, editor_config.font_height);
		Flags extra_flags = (editor_config.editor_pinned) ? UIWindowFlags_NoMove | UIWindowFlags_NoResize : 0;
		UI::Begin("atmos_editor", editor_config.editor_pos, editor_config.editor_size, UIWindowFlags_NoBorder | UIWindowFlags_NoScroll | extra_flags);
		UIStyle&  style  = UI::GetStyle();
		UIWindow* window = UI::GetWindow();
		
		f32 header_height = style.fontHeight * style.buttonHeightRelToFont;
		vec2 window_padding = style.windowPadding;
		UI::PushVar(UIStyleVar_WindowMargins, vec2::ZERO);
		UI::BeginChild("atmos_editor_header", vec2{window->width, header_height}, UIWindowFlags_Invisible | UIWindowFlags_NoBorder | UIWindowFlags_NoScroll | UIWindowFlags_FitAllElements);
		
		//// header ////
		//NOTE editor_menu_bar_tab persists across frames so we can hover test against the popout it makes and the button
		//TODO minimize editor button
		UIItem* menu_item = 0;
		color separator_color = style.colors[UIStyleCol_ButtonBg] * 2.f;
		
		//draw menu bar tabs and check if they are hovered
		UI::PushColor(UIStyleCol_ButtonBorder, Color_Transparent);
		UI::BeginRow("atmos_editor_menubar", 3, header_height);{
			UI::RowSetupRelativeColumnWidths({1,1,1});
			vec2 line_start{0,0}, end_offset{0, header_height};
			
			if(UI::Button("Level")   || UI::IsLastItemHovered()) editor_menu_bar_tab = EditorMenuBarTab_Level;
			if(editor_menu_bar_tab == EditorMenuBarTab_Level) menu_item = UI::GetLastItem();
			line_start = UI::GetLastItemPos() + UI::GetLastItemSize().xComp();
			UI::Line(line_start, line_start + end_offset, 1, separator_color);
			
			if(UI::Button("View") || UI::IsLastItemHovered()) editor_menu_bar_tab = EditorMenuBarTab_View;
			if(editor_menu_bar_tab == EditorMenuBarTab_View) menu_item = UI::GetLastItem();
			line_start = UI::GetLastItemPos() + UI::GetLastItemSize().xComp();
			UI::Line(line_start, line_start + end_offset, 1, separator_color);
			
			if(UI::Button("State")   || UI::IsLastItemHovered()) editor_menu_bar_tab = EditorMenuBarTab_State;
			if(editor_menu_bar_tab == EditorMenuBarTab_State) menu_item = UI::GetLastItem();
			line_start = UI::GetLastItemPos() + UI::GetLastItemSize().xComp();
			UI::Line(line_start, line_start + end_offset, 1, separator_color);
		}UI::EndRow();
		UI::Line(vec2{0,header_height+1}, vec2{window->width,header_height+1}, 1, separator_color);
		UI::PopColor(1);
		
		if(menu_item){
			//draw menu popout
			b32 popout_hovered = true;
			vec2 popout_pos = (menu_item->position + vec2{-1, menu_item->size.y}).floor();
			UI::PushVar(UIStyleVar_ItemSpacing,      vec2::ONE);
			UI::PushVar(UIStyleVar_WindowMargins,    vec2::ZERO);
			UI::PushVar(UIStyleVar_WindowBorderSize, 1.f);
			UI::PushVar(UIStyleVar_ButtonBorderSize, 0);
			UI::PushColor(UIStyleCol_Border,   separator_color);
			UI::PushColor(UIStyleCol_WindowBg, separator_color);
			//NOTE a different popout for each tab until buttons work with fitallelements
			UI::BeginPopOut("atmos_editor_menubar_menus", popout_pos, menu_item->size.ceil() + vec2{1,0}, UIWindowFlags_NoMove | UIWindowFlags_NoScroll | UIWindowFlags_FitAllElements);
			if(editor_menu_bar_tab == EditorMenuBarTab_Level){
				if(UI::Button("New")){
					AtmoAdmin->Reset();
				}
				if(UI::Button("Save")){
					if(AtmoAdmin->levelName == ""){
						editor_show_leveldir = true;
						editor_leveldir_saving = true;
					}else{
						AtmoAdmin->SaveLevel(cstring{AtmoAdmin->levelName.str,upt(AtmoAdmin->levelName.count)});
					}
				}
				if(UI::Button("Save As")){
					editor_show_leveldir = true;
					editor_leveldir_saving = true;
				}
				if(UI::Button("Load")){
					editor_show_leveldir = true;
					editor_leveldir_saving = false;
				}
			}else if(editor_menu_bar_tab == EditorMenuBarTab_View){
				if(UI::Button("Metrics")) ToggleBool(editor_config.draw_metrics);
				if(UI::Button("Storage")) ToggleBool(editor_config.draw_storage);
				if(UI::Button("UI Demo")) ToggleBool(editor_config.draw_ui_demo);
			}else if(editor_menu_bar_tab == EditorMenuBarTab_State){
				
			}
			if(!UI::IsWinHovered()) popout_hovered = false;
			UI::EndPopOut();
			UI::PopColor(2); UI::PopVar(4);
			
			//if neither the popout is hovered nor the menu bar tab is hovered, stop drawing the menu popouts next frame
			vec2 hover_test_pos = window->position + menu_item->position + vec2{-1,0};
			vec2 hover_test_dim = menu_item->size + vec2{1,header_height};
			if(!(popout_hovered || Math::PointInRectangle(DeshInput->mousePos, hover_test_pos, hover_test_dim))){
				editor_menu_bar_tab = EditorMenuBarTab_NONE;
			}
		}
		
		UI::EndChild();
		UI::BeginChild("atmos_editor_body", vec2{window->width, window->height - (2*header_height) - 2}, UIWindowFlags_Invisible | UIWindowFlags_NoBorder | UIWindowFlags_NoScroll);
		UI::PushVar(UIStyleVar_WindowMargins, window_padding);
		
		//// tabs ////
		enum EditorTabs{
			EditorTab_Scene,
			EditorTab_Undos,
			EditorTab_Config,
		} editor_tab = EditorTab_Scene;
		UI::BeginTabBar("atmos_editor_tabbar", UITabBarFlags_NoIndent);{
			if(UI::BeginTab("Scene")) { editor_tab = EditorTab_Scene;  UI::EndTab(); }
			if(UI::BeginTab("Undos")) { editor_tab = EditorTab_Undos;  UI::EndTab(); }
			if(UI::BeginTab("Config")){ editor_tab = EditorTab_Config; UI::EndTab(); }
		}UI::EndTabBar();
		
		//// @scene_ui ////
		//TODO entity list
		//TODO entity editor
		
		//// @undos_ui ////
		//TODO undo list
		//TODO redo list
		//TODO multiple undo/redo
		
		//// @config_ui ////
		if(editor_tab == EditorTab_Config){
			if(UI::BeginHeader("Physics")){
				UI::Text("Simulate in Editor "); UI::SameLine(); UI::SetNextItemWidth(MAX_F32); BoolButton(AtmoAdmin->simulateInEditor);
				UI::Text("Simulation Paused  "); UI::SameLine(); UI::SetNextItemWidth(MAX_F32); BoolButton(AtmoAdmin->physics.paused);
				UI::Text("Integration        "); UI::SameLine(); UI::SetNextItemWidth(MAX_F32); BoolButton(AtmoAdmin->physics.integrating);
				UI::Text("Manifold Solving   "); UI::SameLine(); UI::SetNextItemWidth(MAX_F32); BoolButton(AtmoAdmin->physics.solving);
				UI::SetNextItemWidth(MAX_F32);
				if(UI::Button("Step Once")){ AtmoAdmin->physics.solving = true; }
				UI::Text("Gravity              "); UI::SameLine(); InputFloat("editor_phys_gravity",   &AtmoAdmin->physics.gravity);
				UI::Text("Min Linear Velocity  "); UI::SameLine(); InputFloat("editor_phys_minlinvel", &AtmoAdmin->physics.minVelocity);
				UI::Text("Max Linear Velocity  "); UI::SameLine(); InputFloat("editor_phys_maxlinvel", &AtmoAdmin->physics.maxVelocity);
				UI::Text("Min Angular Velocity "); UI::SameLine(); InputFloat("editor_phys_minangvel", &AtmoAdmin->physics.minRotVelocity);
				UI::Text("Max Angular Velocity "); UI::SameLine(); InputFloat("editor_phys_maxangvel", &AtmoAdmin->physics.maxRotVelocity);
				UI::EndHeader();
			}
			
			if(UI::BeginHeader("Camera")){
				UI::BeginRow("atmos_editor_cam_buttons", 2, style.fontHeight*style.buttonHeightRelToFont);{
					UI::RowSetupRelativeColumnWidths({1,1});
					if(UI::Button("Zero")) { AtmoAdmin->camera.position = vec3::ZERO; AtmoAdmin->camera.rotation = vec3::ZERO; }
					if(UI::Button("Reset")){ AtmoAdmin->camera.position = { 4.f,3.f,-4.f }; AtmoAdmin->camera.rotation = { 28.f,-45.f,0.f }; }
				}UI::EndRow();
				UI::Text("Position  "); UI::SameLine(); InputVec3("editor_cam_pos", &AtmoAdmin->camera.position);
				UI::Text("Rotation  "); UI::SameLine(); InputVec3("editor_cam_rot", &AtmoAdmin->camera.rotation);
				UI::Text("Near Clip "); UI::SameLine(); if(InputFloat("editor_cam_nearz", &AtmoAdmin->camera.nearZ)) AtmoAdmin->camera.UpdateProjectionMatrix();
				UI::Text("Far Clip  "); UI::SameLine(); if(InputFloat("editor_cam_farz", &AtmoAdmin->camera.farZ)) AtmoAdmin->camera.UpdateProjectionMatrix();
				UI::Text("hFOV      "); UI::SameLine(); if(InputFloat("editor_cam_fov", &AtmoAdmin->camera.fov)) AtmoAdmin->camera.UpdateProjectionMatrix();
				UI::EndHeader();
			}
			
			if(UI::BeginHeader("Rendering")){
				persist RenderSettings* settings = Render::GetSettings();
				persist const char* resolution_strings[] = { "128", "256", "512", "1024", "2048", "4096" };
				persist u32 resolution_values[] = { 128, 256, 512, 1024, 2048, 4096 };
				persist u32 shadow_resolution_index = 4;
				persist const char* msaa_strings[] = { "1", "2", "4", "8", "16", "32", "64" };
				persist vec3 clear_color = settings->clearColor;
				persist vec4 selected_color = settings->selectedColor;
				persist vec4 collider_color = settings->colliderColor;
				
				UI::Text("Debugging            "); UI::SameLine(); UI::SetNextItemWidth(MAX_F32); BoolButton(settings->debugging);
				UI::Text("Shader Printf        "); UI::SameLine(); UI::SetNextItemWidth(MAX_F32); BoolButton(settings->printf);
				UI::Text("Recompile Shaders    "); UI::SameLine(); UI::SetNextItemWidth(MAX_F32); BoolButton(settings->recompileAllShaders);
				UI::Text("MSAA Samples         "); UI::SameLine(); UI::SetNextItemWidth(MAX_F32);
				if(UI::BeginCombo("atmos_editor_msaa_combo", msaa_strings[settings->msaaSamples])){
					forI(ArrayCount(msaa_strings)){
						if(UI::Selectable(msaa_strings[i], settings->msaaSamples == i)){
							settings->msaaSamples = i;
						}
					}
					UI::EndCombo(); //atmos_editor_msaa_combo
				}
				UI::Text("Texture Filtering    "); UI::SameLine(); UI::SetNextItemWidth(MAX_F32); BoolButton(settings->textureFiltering);
				UI::Text("Anistropic Filtering "); UI::SameLine(); UI::SetNextItemWidth(MAX_F32); BoolButton(settings->anistropicFiltering);
				TextCentered("^ above settings require restart ^"); //TODO fix TextCentered
				UI::Text("Logging Level        "); UI::SameLine(); UI::Text("TODO InputInt"); //UI::SetNextItemWidth(MAX_F32);
				UI::Text("Crash On Error       "); UI::SameLine(); UI::SetNextItemWidth(MAX_F32); BoolButton(settings->crashOnError);
				UI::Text("Shader Optimization  "); UI::SameLine(); UI::SetNextItemWidth(MAX_F32); BoolButton(settings->optimizeShaders); //TODO note on update
				UI::Text("Shadow PCF           "); UI::SameLine(); UI::SetNextItemWidth(MAX_F32); BoolButton(settings->shadowPCF);
				UI::Text("Shadowmap Resolution "); UI::SameLine(); UI::SetNextItemWidth(MAX_F32);
				if(UI::BeginCombo("atmos_editor_sahdowres_combo", resolution_strings[shadow_resolution_index])){
					forI(ArrayCount(resolution_strings)){
						if(UI::Selectable(resolution_strings[i], shadow_resolution_index == i)){
							settings->shadowResolution = resolution_values[i];
							shadow_resolution_index = i;
							Render::remakeOffscreen();
						}
					}
					UI::EndCombo(); //atmos_editor_msaa_combo
				}
				UI::Text("Shadow near clip     "); UI::SameLine(); InputFloat("editor_shadow_near", &settings->shadowNearZ);
				UI::Text("Shadow far  clip     "); UI::SameLine(); InputFloat("editor_shadow_far", &settings->shadowFarZ);
				UI::Text("Shadow depth bias constant "); UI::SameLine(); InputFloat("editor_shadow_contant", &settings->depthBiasConstant);
				UI::Text("Shadow depth bias slope "); UI::SameLine(); InputFloat("editor_shadow_slope", &settings->depthBiasSlope);
				UI::Text("Show shadowmap texture "); UI::SameLine(); UI::SetNextItemWidth(MAX_F32); BoolButton(settings->showShadowMap);
				UI::Text("Background Color     "); UI::SameLine(); UI::Text("TODO InputColor");
				UI::Text("Selected   Color     "); UI::SameLine(); UI::Text("TODO InputColor");
				UI::Text("Collider   Color     "); UI::SameLine(); UI::Text("TODO InputColor");
				UI::Text("Only show wireframe  "); UI::SameLine(); UI::SetNextItemWidth(MAX_F32); BoolButton(settings->wireframeOnly);
				UI::Text("Draw mesh wireframes "); UI::SameLine(); UI::SetNextItemWidth(MAX_F32); BoolButton(settings->meshWireframes);
				UI::Text("Draw mesh normals    "); UI::SameLine(); UI::SetNextItemWidth(MAX_F32); BoolButton(settings->meshNormals);
				UI::Text("Draw light frustrums "); UI::SameLine(); UI::SetNextItemWidth(MAX_F32); BoolButton(settings->lightFrustrums);
				UI::Text("Draw temp meshes on top "); UI::SameLine(); UI::SetNextItemWidth(MAX_F32); BoolButton(settings->tempMeshOnTop);
				UI::SetNextItemWidth(MAX_F32);
				if(UI::Button("Clear Debug Meshes")){ Render::ClearDebug(); }
				UI::EndHeader();
			}
		}
		
		UI::PopVar();
		UI::EndChild();
		UI::BeginChild("atmos_editor_footer", vec2{window->width, header_height}, UIWindowFlags_Invisible | UIWindowFlags_NoBorder | UIWindowFlags_NoScroll | UIWindowFlags_FitAllElements);
		
		//// footer ////
		//TODO average fps and plot
		//TODO real life time
		
		UI::EndChild();
		UI::End();
		UI::PopVar(2);
		UI::PopFont();
	}
	
	//// @leveldir_ui ////
	if(editor_show_leveldir){
		UIStyle& style = UI::GetStyle();
		
		vec2 window_size = vec2{DeshWindow->width/5.f,DeshWindow->width/5.f}.floor();
		UI::Begin("atmos_editor_leveldir", vec2(DeshWindow->centerX,DeshWindow->centerY) - window_size/2.f, window_size, UIWindowFlags_NoScroll);
		vec2 input_size = vec2{UI::GetMarginedArea().second.x+1, style.fontHeight * style.inputTextHeightRelToFont};
		
		UI::SetNextItemSize(input_size);
		UI::InputText("atmos_editor_leveldir_path", editor_leveldir_dir_buffer,  sizeof(editor_leveldir_dir_buffer));
		UI::SetNextItemSize(input_size);
		UI::InputText("atmos_editor_leveldir_file", editor_leveldir_file_buffer, sizeof(editor_leveldir_file_buffer), "example.level");
		string full_path(editor_leveldir_dir_buffer,  deshi_temp_allocator);
		string file_name(editor_leveldir_file_buffer, deshi_temp_allocator);
		if(full_path[full_path.count-1] != '/' || full_path[full_path.count-1] != '\\') full_path += "/";
		full_path += file_name;
		
		vec2 button_size = vec2{UI::GetMarginedArea().second.x/2.f, style.fontHeight * style.buttonHeightRelToFont};
		b32 try_operation = false;
		b32 file_already_exists = file_exists(full_path.str) && file_name.count;
		UI::SetNextItemSize(button_size);
		if(editor_leveldir_saving){
			if(UI::Button((file_already_exists) ? "Overwrite" : "Save")) try_operation = true;
		}else{
			if(file_already_exists){
				if(UI::Button("Load")) try_operation = true;
			}else{
				StartDisabled();
				UI::Button("Load");
				StopDisabled();
			}
		}
		UI::SameLine();
		UI::SetNextItemSize(button_size);
		if(UI::Button("Cancel")){
			editor_show_leveldir = false;
			editor_leveldir_saving = false;
		}
		
		UI::SetWinCursorY(UI::GetWinCursor().y + style.fontHeight * style.buttonHeightRelToFont);
		vec2 list_dims = (UI::GetMarginedArea().second - UI::GetWinCursor().yComp()).floor();
		UI::SetNextWindowSize(list_dims);
		UI::BeginChild("atmos_editor_leveldir_list", list_dims, UIWindowFlags_NoResize | UIWindowFlags_NoMove | UIWindowFlags_NoScrollX);{
			UI::PushVar(UIStyleVar_WindowMargins,    vec2::ZERO);
			UI::PushVar(UIStyleVar_WindowBorderSize, 0.f);
			UI::PushVar(UIStyleVar_ButtonBorderSize, 1.f);
			UI::PushVar(UIStyleVar_ButtonTextAlign,  vec2{0, .5f});
			
			button_size = vec2{UI::GetClientArea().second.x - 1, style.fontHeight * style.buttonHeightRelToFont}.floor();
			if(file_exists(editor_leveldir_dir_buffer)){
				File* directory = file_info(editor_leveldir_dir_buffer);
				if(directory && directory->is_directory){
					UI::SetNextItemSize(button_size);
					if(UI::Button("..")){
						u32 slash_idx = 0;
						if(directory->path[directory->path_length-1] == '/' || directory->path[directory->path_length-1] == '\\'){
							slash_idx = find_last_char(directory->path, '/', 1);
							if(slash_idx == -1) slash_idx = find_last_char(directory->path, '\\', 1);
						}else{
							slash_idx = find_last_char(directory->path, '/');
							if(slash_idx == -1) slash_idx = find_last_char(directory->path, '\\');
						}
						Assert(slash_idx != -1);
						editor_leveldir_dir_buffer[slash_idx] = '\0';
					}
					
					//TODO reuse list_file_name var
					array<File> files = get_directory_files(directory);
					forE(files){ //directories first
						if(it->is_directory){
							string list_file_name(get_file_name(*it), deshi_temp_allocator);
							UI::SetNextItemSize(button_size);
							if(UI::Button(list_file_name.str)){
								CopyMemory(editor_leveldir_dir_buffer, it->path, it->path_length+1);
								ZeroMemory(editor_leveldir_file_buffer, sizeof(editor_leveldir_file_buffer));
							}
						}
					}
					forE(files){ //files after
						if(!it->is_directory){
							string list_file_name(get_file_name(*it), deshi_temp_allocator);
							UI::SetNextItemSize(button_size);
							if(UI::Button(list_file_name.str)){
								CopyMemory(editor_leveldir_file_buffer, list_file_name.str, list_file_name.count+1);
							}
						}
					}
				}else{
					UI::Text("Invalid directory");
				}
			}
			
			UI::PopVar(4);
		}UI::EndChild();
		
		if(try_operation){ //NOTE this only happens if the filepath has already been verified as valid
			u32 dot_idx = find_last_char(file_name, '.');
			if(editor_leveldir_saving){
				AtmoAdmin->SaveLevel(cstring{file_name.str, dot_idx});
			}else{
				AtmoAdmin->LoadLevel(cstring{file_name.str, dot_idx});
			}
			editor_show_leveldir = false;
			editor_leveldir_saving = false;
		}
		
		UI::End();
	}
	
	//// @ui_other ////
	if(editor_config.draw_metrics) UI::ShowMetricsWindow();
	if(editor_config.draw_storage) Storage::StorageBrowserUI();
	if(editor_config.draw_ui_demo) UI::DemoWindow();
}

#undef BoolButton
#endif //ATMOS_IMPLEMENTATION