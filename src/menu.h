#pragma once
#ifndef ATMOS_MENU_H
#define ATMOS_MENU_H

void InitMenu();
void UpdateMenu();

#endif //ATMOS_MENU_H
#ifdef ATMOS_IMPLEMENTATION

enum MenuOption{
	MenuOption_Resume,
	MenuOption_Quit,
	MenuOption_COUNT,
};

Font* title_font = 0;
Font* option_font = 0;
int active_option = MenuOption_Resume;
b32 option_pressed = false;
b32 ask_for_quit_confirmation = false;
color selected_color = Color_LightYellow;
color regular_color = Color_Grey;

void InitMenu(){
	title_font = Storage::CreateFontFromFileTTF("Mothership.ttf", 128).second;
	option_font = Storage::CreateFontFromFileTTF("marspolice.ttf", 72).second;
}

void UpdateMenu(){
	option_pressed = false;
	
	//// handle input ////
	if(DeshInput->KeyPressed(Key::ENTER)){ 
		option_pressed = true;
	}
	if(DeshInput->KeyPressed(Key::UP)){
		ask_for_quit_confirmation = false;
		active_option--;
		if(active_option < 0) active_option = MenuOption_COUNT-1;
	}
	if(DeshInput->KeyPressed(Key::DOWN)){
		ask_for_quit_confirmation = false;
		active_option++;
		if(active_option >= MenuOption_COUNT) active_option = 0;
	}
	
	if(option_pressed){
		switch(active_option){
			case MenuOption_Resume:{
				AtmoAdmin->ChangeState(GameState_Play);
			}break;
			case MenuOption_Quit:{
				if(ask_for_quit_confirmation){
					DeshWindow->Close();
				}else{
					ask_for_quit_confirmation = true;
				}
			}break;
		}
	}
	
	//// draw menu ////
	UI::PushColor(UIStyleCol_WindowBg, PackColorU32(128,128,255,64));
	UI::Begin("atmos_menu", vec2::ZERO, DeshWindow->dimensions, UIWindowFlags_NoMove|UIWindowFlags_NoTitleBar|UIWindowFlags_NoResize);
	
	f32 center_x = DeshWindow->centerX;
	cstring text;
	
	//draw title
	UI::PushFont(title_font);
	text = cstr_lit("Atmos");
	UI::Text(text.str, {center_x-(UI::CalcTextSize(text).x/2.f),DeshWindow->height*.2f});
	
	//draw options
	UI::PushFont(option_font);
	f32 cursor_y = DeshWindow->height*.2f + title_font->max_height*1.5f;
	f32 stride = option_font->max_height * 1.5f;
	
	text = cstr_lit("Resume");
	UI::PushColor(UIStyleCol_Text, (active_option == MenuOption_Resume) ? selected_color : regular_color);
	UI::Text(text.str, {center_x-(UI::CalcTextSize(text).x/2.f),cursor_y});
	UI::PopColor();
	cursor_y += stride;
	
	text = cstr_lit("Quit");
	if(ask_for_quit_confirmation) text = cstr_lit("Quit? Are you sure?");
	UI::PushColor(UIStyleCol_Text, (active_option == MenuOption_Quit) ? selected_color : regular_color);
	UI::Text(text.str, {center_x-(UI::CalcTextSize(text).x/2.f),cursor_y});
	UI::PopColor();
	cursor_y += stride;
	
	UI::End();
	UI::PopColor();
	UI::PopFont(2);
}

#endif //ATMOS_IMPLEMENTATION