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
@saveas_ui
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

struct EditorConfig{      //defaults
	Font* font;             //gohufont-11.bdf
	f32   font_height;      //11
	b32   editor_minimized; //false
	vec2  editor_pos;       //{0, 0}
	vec2  editor_size;      //{window width / 5, window height}
	b32   draw_grid;        //true
	b32   draw_metrics;     //false
	b32   draw_storage;     //false
	b32   draw_ui_demo;     //false
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

Type editor_transform_type = TransformType_NONE;
Type editor_menu_bar_tab = EditorMenuBarTab_NONE;
b32  editor_show_saveas = false;
char editor_level_saveas_buffer[256] = {};

EditorConfig editor_config;
array<Entity*> editor_selected_entities;


///////////////
//// @init ////
///////////////
void
InitEditor(){
	editor_config.font        = Storage::CreateFontFromFileBDF("gohufont-11.bdf").second;
	editor_config.font_height = editor_config.font->max_height;
	editor_config.editor_minimized = false;
	editor_config.editor_pos       = vec2::ZERO;
	editor_config.editor_size      = vec2{f32(DeshWindow->width)/5.f, f32(DeshWindow->height)};
	editor_config.draw_grid    = true;
	editor_config.draw_metrics = false;
	editor_config.draw_storage = false;
	editor_config.draw_ui_demo = false;
	
	//TODO load editor config file
	
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
void
UpdateEditor(){
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
				editor_show_saveas = true;
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
	{
		//TODO world grid
	}
	
	//// @ui_minimized ////
	if(editor_config.editor_minimized && !DeshWindow->minimized){
		//TODO minimized editor
	}
	
	//// @ui_expanded ////
	if(!editor_config.editor_minimized && !DeshWindow->minimized){
		UI::PushFont(editor_config.font);
		UI::PushVar(UIStyleVar_FontHeight, editor_config.font_height);
		UI::Begin("atmos_editor", editor_config.editor_pos, editor_config.editor_size, UIWindowFlags_NoBorder | UIWindowFlags_NoScroll);
		UIStyle&  style  = UI::GetStyle();
		UIWindow* window = UI::GetWindow();
		
		f32 header_height = style.fontHeight * style.buttonHeightRelToFont;
		vec2 window_padding = style.windowPadding;
		UI::PushVar(UIStyleVar_WindowPadding, vec2::ZERO);
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
			if(editor_menu_bar_tab == EditorMenuBarTab_Level)   menu_item = UI::GetLastItem();
			line_start = UI::GetLastItemPos() + UI::GetLastItemSize().xComp();
			UI::Line(line_start, line_start + end_offset, 1, separator_color);
			
			if(UI::Button("View") || UI::IsLastItemHovered()) editor_menu_bar_tab = EditorMenuBarTab_View;
			if(editor_menu_bar_tab == EditorMenuBarTab_View) menu_item = UI::GetLastItem();
			line_start = UI::GetLastItemPos() + UI::GetLastItemSize().xComp();
			UI::Line(line_start, line_start + end_offset, 1, separator_color);
			
			if(UI::Button("State")   || UI::IsLastItemHovered()) editor_menu_bar_tab = EditorMenuBarTab_State;
			if(editor_menu_bar_tab == EditorMenuBarTab_State)   menu_item = UI::GetLastItem();
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
			UI::PushVar(UIStyleVar_WindowPadding,    vec2::ZERO);
			UI::PushVar(UIStyleVar_WindowBorderSize, 1.f);
			UI::PushVar(UIStyleVar_ButtonBorderSize, 0);
			UI::PushColor(UIStyleCol_Border,   separator_color);
			UI::PushColor(UIStyleCol_WindowBg, separator_color);
			if(editor_menu_bar_tab == EditorMenuBarTab_Level){
				UI::BeginPopOut("atmos_editor_menubar_level", popout_pos, menu_item->size, UIWindowFlags_NoMove | UIWindowFlags_NoScroll | UIWindowFlags_FitAllElements);
				
				if(UI::Button("New")){
					AtmoAdmin->Reset();
				}
				if(UI::Button("Save")){
					if(AtmoAdmin->levelName == ""){
						editor_show_saveas = true;
					}else{;
						AtmoAdmin->SaveLevel(cstring{AtmoAdmin->levelName.str,upt(AtmoAdmin->levelName.count)});
					}
				}
				
				//NOTE temporary disabled colors until save as and load are implemented
				UI::PushColor(UIStyleCol_ButtonBg,        Color_Black);
				UI::PushColor(UIStyleCol_ButtonBgActive,  Color_Black);
				UI::PushColor(UIStyleCol_ButtonBgHovered, Color_Black);
				UI::PushColor(UIStyleCol_Text,            Color_DarkGrey);
				if(UI::Button("Save As")){
					editor_show_saveas = true;
				}
				
				if(UI::Button("Load")){
					
				}
				UI::PopColor(4);
				
				if(!UI::IsWinHovered()) popout_hovered = false;
				UI::EndPopOut();
			}else if(editor_menu_bar_tab == EditorMenuBarTab_View){
				UI::BeginPopOut("atmos_editor_menubar_view", popout_pos, menu_item->size, UIWindowFlags_NoMove | UIWindowFlags_NoScroll | UIWindowFlags_FitAllElements);
				
				if(UI::Button("Metrics")) ToggleBool(editor_config.draw_metrics);
				if(UI::Button("Storage")) ToggleBool(editor_config.draw_storage);
				if(UI::Button("UI Demo")) ToggleBool(editor_config.draw_ui_demo);
				
				if(!UI::IsWinHovered()) popout_hovered = false;
				UI::EndPopOut();
			}else if(editor_menu_bar_tab == EditorMenuBarTab_State){
				
			}
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
		UI::PushVar(UIStyleVar_WindowPadding, window_padding);
		
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
		//TODO config list
		
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
	
	//// @saveas_ui ////
	if(false && editor_show_saveas){
		UIWindow* popout = 0;
		//TODO lighten everything except this window
		UI::Begin("atmos_editor_saveas", vec2::ZERO, vec2{MAX_F32,MAX_F32}, UIWindowFlags_NoScroll | UIWindowFlags_FitAllElements);
		//TODO saveas
		UI::End();
	}
	
	//// @ui_other ////
	if(editor_config.draw_metrics) UI::ShowMetricsWindow();
	if(editor_config.draw_storage) Storage::StorageBrowserUI();
	if(editor_config.draw_ui_demo) UI::DemoWindow();
}

#endif //ATMOS_IMPLEMENTATION