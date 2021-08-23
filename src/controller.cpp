#include "controller.h"
#include "Admin.h"
#include "camerainstance.h"
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
	
    //camera movement
	if(DeshInput->KeyDownAnyMod(MouseButton::RIGHT)){
		if(DeshInput->KeyDownAnyMod(movementFlyingUp))     { inputs.y += 1; }
		if(DeshInput->KeyDownAnyMod(movementFlyingDown))   { inputs.y -= 1; }
		if(DeshInput->KeyDownAnyMod(movementFlyingForward)){ inputs += AtmoAdmin->camera.forward; }
		if(DeshInput->KeyDownAnyMod(movementFlyingBack))   { inputs -= AtmoAdmin->camera.forward; }
		if(DeshInput->KeyDownAnyMod(movementFlyingRight))  { inputs += AtmoAdmin->camera.right; }
		if(DeshInput->KeyDownAnyMod(movementFlyingLeft))   { inputs -= AtmoAdmin->camera.right; }
        
		if     (DeshInput->LShiftDown()){ AtmoAdmin->camera.position += inputs.normalized() * 16.f * DeshTime->deltaTime; }
		else if(DeshInput->LCtrlDown()) { AtmoAdmin->camera.position += inputs.normalized() * 4.f  * DeshTime->deltaTime; }
		else                            { AtmoAdmin->camera.position += inputs.normalized() * 8.f  * DeshTime->deltaTime; }
	}
    
	//camera rotation
	if(DeshInput->KeyPressedAnyMod(MouseButton::RIGHT)){ DeshWindow->UpdateCursorMode(CursorMode_FirstPerson); }
	if(DeshInput->KeyReleasedAnyMod(MouseButton::RIGHT)){ DeshWindow->UpdateCursorMode(CursorMode_Default); }
	if(DeshInput->KeyDownAnyMod(MouseButton::RIGHT)){
		AtmoAdmin->camera.rotation.y += (DeshInput->mouseX - DeshWindow->centerX) * cameraSensitivity * MOUSE_SENS_FRACTION;
		AtmoAdmin->camera.rotation.x += (DeshInput->mouseY - DeshWindow->centerY) * cameraSensitivity * MOUSE_SENS_FRACTION;
	}
}

void Controller::Save(){
	Assets::saveConfig("keybinds.cfg", keybindMap);
}

void Controller::Load(){
	Assets::loadConfig("keybinds.cfg", keybindMap);
}
