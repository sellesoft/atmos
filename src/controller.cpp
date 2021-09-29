#include "controller.h"
#include "Admin.h"
#include "camerainstance.h"
#include "entities/PlayerEntity.h"
#include "core/window.h"

local f32 MOUSE_SENS_FRACTION = .03f; //TODO(delle) calculate this to be the same as Source

void Controller::Init(){
	keybindMap = {
		{"#keybinds config file",0,0},
		{"\n#movement",ConfigValueType_PADSECTION,(void*)24},
		{"movement_flying_up",      ConfigValueType_Key, &movementFlyingUp},
		{"movement_flying_down",    ConfigValueType_Key, &movementFlyingDown},
		{"movement_flying_forward", ConfigValueType_Key, &movementFlyingForward},
		{"movement_flying_back",    ConfigValueType_Key, &movementFlyingBack},
		{"movement_flying_left",    ConfigValueType_Key, &movementFlyingRight},
		{"movement_flying_right",   ConfigValueType_Key, &movementFlyingLeft},
		{"\n#camera",ConfigValueType_PADSECTION,(void*)27},
		{"camera_sens",                ConfigValueType_F32, &cameraSensitivity},
		{"camera_rotate_up",           ConfigValueType_Key, &cameraRotateUp},
		{"camera_rotate_down",         ConfigValueType_Key, &cameraRotateDown},
		{"camera_rotate_left",         ConfigValueType_Key, &cameraRotateLeft},
		{"camera_rotate_right",        ConfigValueType_Key, &cameraRotateRight},
		{"camera_perspective_toggle",  ConfigValueType_Key, &perspectiveToggle},
		{"camera_goto_selected",       ConfigValueType_Key, &gotoSelected},
		{"camera_ortho_offset",        ConfigValueType_Key, &orthoOffset},
		{"camera_ortho_reset_offset",  ConfigValueType_Key, &orthoResetOffset},
		{"camera_ortho_zoom_in",       ConfigValueType_Key, &orthoZoomIn},
		{"camera_ortho_zoom_out",      ConfigValueType_Key, &orthoZoomOut},
		{"camera_ortho_right_view",    ConfigValueType_Key, &orthoRightView},
		{"camera_ortho_left_view",     ConfigValueType_Key, &orthoLeftView},
		{"camera_ortho_front_view",    ConfigValueType_Key, &orthoFrontView},
		{"camera_ortho_back_view",     ConfigValueType_Key, &orthoBackView},
		{"camera_ortho_topdown_view",  ConfigValueType_Key, &orthoTopDownView},
		{"camera_ortho_bottomup_view", ConfigValueType_Key, &orthoBottomUpView},
		{"\n#editor",ConfigValueType_PADSECTION,(void*)26},
		{"editor_undo",               ConfigValueType_Key, &undo},
		{"editor_redo",               ConfigValueType_Key, &redo},
		{"editor_translate_selected", ConfigValueType_Key, &grabSelectedObject},
		{"editor_rotate_selected",    ConfigValueType_Key, &rotateSelectedObject},
		{"editor_scale_selected",     ConfigValueType_Key, &scaleSelectedObject},
		{"\n#gui",ConfigValueType_PADSECTION,(void*)21},
		{"gui_console_toggle",   ConfigValueType_Key, &toggleConsole},
		{"gui_debugmenu_toggle", ConfigValueType_Key, &toggleDebugMenu},
		{"gui_debugbar_toggle",  ConfigValueType_Key, &toggleDebugBar},
		{"gui_menubar_toggle",   ConfigValueType_Key, &toggleMenuBar},
	};
	
	Load();
}

void Controller::Update(){
	vec3 inputs = vec3::ZERO;
	
	switch(AtmoAdmin->state){
		case GameState_Play:{
			//game state
			if(DeshInput->KeyPressed(Key::ESCAPE)) AtmoAdmin->ChangeState(GameState_Menu);
			if(DeshInput->KeyPressed(Key::F10))    AtmoAdmin->ChangeState(GameState_Editor);
			
			if(DeshInput->KeyDown(movementWalkingForward)) { inputs += vec3(AtmoAdmin->camera.forward.x, 0, AtmoAdmin->camera.forward.z); }
			if(DeshInput->KeyDown(movementWalkingBackward)){ inputs -= vec3(AtmoAdmin->camera.forward.x, 0, AtmoAdmin->camera.forward.z); }
			if(DeshInput->KeyDown(movementWalkingRight))   { inputs += vec3(AtmoAdmin->camera.right.x, 0, AtmoAdmin->camera.right.z); }
			if(DeshInput->KeyDown(movementWalkingLeft))    { inputs -= vec3(AtmoAdmin->camera.right.x, 0, AtmoAdmin->camera.right.z); }
			
			if(DeshInput->KeyPressed (movementJump))  { AtmoAdmin->player->isJumping   = true; }
			if(DeshInput->KeyReleased(movementJump))  { AtmoAdmin->player->isJumping   = false; }
			if(DeshInput->KeyPressed (movementCrouch)){ AtmoAdmin->player->isCrouching = true; }
			if(DeshInput->KeyReleased(movementCrouch)){ AtmoAdmin->player->isCrouching = false; }
			if(DeshInput->KeyPressed (movementRun))   { AtmoAdmin->player->isRunning   = true; }
			if(DeshInput->KeyReleased(movementRun))   { AtmoAdmin->player->isRunning   = false; }
			
			AtmoAdmin->camera.rotation.y += (DeshInput->mouseX - DeshWindow->centerX) * cameraSensitivity * MOUSE_SENS_FRACTION;
			AtmoAdmin->camera.rotation.x += (DeshInput->mouseY - DeshWindow->centerY) * cameraSensitivity * MOUSE_SENS_FRACTION;
			AtmoAdmin->player->inputs = inputs.normalized();
		}break;
		
		case GameState_Menu:{
			//game state
			if(DeshInput->KeyPressed(Key::ESCAPE)) AtmoAdmin->ChangeState(GameState_Play);
			
			
		}break;
		
		case GameState_Editor:{
			//game state
			if(DeshInput->KeyPressed(Key::F10)) AtmoAdmin->ChangeState(GameState_Play);
			
			//camera movement
			if(DeshInput->KeyDown(MouseButton::RIGHT)){
				if(DeshInput->KeyDown(movementFlyingUp))     { inputs.y += 1; }
				if(DeshInput->KeyDown(movementFlyingDown))   { inputs.y -= 1; }
				if(DeshInput->KeyDown(movementFlyingForward)){ inputs += AtmoAdmin->camera.forward; }
				if(DeshInput->KeyDown(movementFlyingBack))   { inputs -= AtmoAdmin->camera.forward; }
				if(DeshInput->KeyDown(movementFlyingRight))  { inputs += AtmoAdmin->camera.right; }
				if(DeshInput->KeyDown(movementFlyingLeft))   { inputs -= AtmoAdmin->camera.right; }
				
				if     (DeshInput->LShiftDown()){ AtmoAdmin->camera.position += inputs.normalized() * 32.f * DeshTime->deltaTime; }
				else if(DeshInput->LCtrlDown()) { AtmoAdmin->camera.position += inputs.normalized() * 4.f  * DeshTime->deltaTime; }
				else                            { AtmoAdmin->camera.position += inputs.normalized() * 8.f  * DeshTime->deltaTime; }
			}
			
			//camera rotation
			if(DeshInput->KeyPressed(MouseButton::RIGHT)){ DeshWindow->UpdateCursorMode(CursorMode_FirstPerson); }
			if(DeshInput->KeyReleased(MouseButton::RIGHT)){ DeshWindow->UpdateCursorMode(CursorMode_Default); }
			if(DeshInput->KeyDown(MouseButton::RIGHT)){
				AtmoAdmin->camera.rotation.y += (DeshInput->mouseX - DeshWindow->centerX) * cameraSensitivity * MOUSE_SENS_FRACTION;
				AtmoAdmin->camera.rotation.x += (DeshInput->mouseY - DeshWindow->centerY) * cameraSensitivity * MOUSE_SENS_FRACTION;
			}
		}break;
	}
}

void Controller::Save(){
	Assets::saveConfig("keybinds.cfg", keybindMap);
}

void Controller::Load(){
	Assets::loadConfig("keybinds.cfg", keybindMap);
}
