#pragma once
#ifndef ATMOS_CONTROLLER_H
#define ATMOS_CONTROLLER_H

#include "defines.h"
#include "core/input.h"
#include "core/assets.h"

struct CameraInstance;
struct Controller{
    //flying movement
	Key::Key movementFlyingUp      = Key::E;
	Key::Key movementFlyingDown    = Key::Q;
	Key::Key movementFlyingForward = Key::W;
	Key::Key movementFlyingBack    = Key::S;
	Key::Key movementFlyingRight   = Key::D;
	Key::Key movementFlyingLeft    = Key::A;
    
	//walking movement
	Key::Key movementWalkingForward  = Key::W;
	Key::Key movementWalkingBackward = Key::S;
	Key::Key movementWalkingRight    = Key::D;
	Key::Key movementWalkingLeft     = Key::A;
	Key::Key movementJump            = Key::SPACE;
	Key::Key movementCrouch          = Key::LCTRL;
	Key::Key movementRun             = Key::LSHIFT;
    
	//player controls
	Key::Key use = Key::E;
    
	//camera controls
    f32 cameraSensitivity      = 2.5f;
	Key::Key cameraRotateUp    = Key::UP;
	Key::Key cameraRotateDown  = Key::DOWN;
	Key::Key cameraRotateRight = Key::RIGHT;
	Key::Key cameraRotateLeft  = Key::LEFT;
	Key::Key orthoOffset       = MouseButton::MIDDLE;
	Key::Key orthoZoomIn       = MouseButton::SCROLLUP;
	Key::Key orthoZoomOut      = MouseButton::SCROLLDOWN;
	Key::Key orthoResetOffset  = Key::NUMPADPERIOD;
	Key::Key orthoRightView    = Key::NUMPAD6;
	Key::Key orthoLeftView     = Key::NUMPAD4;
	Key::Key orthoFrontView    = Key::NUMPAD8;
	Key::Key orthoBackView     = Key::NUMPAD2;
	Key::Key orthoTopDownView  = Key::NUMPAD1;
	Key::Key orthoBottomUpView = Key::NUMPAD3;
	Key::Key perspectiveToggle = Key::NUMPAD0;
	Key::Key gotoSelected      = Key::NUMPADENTER;
    
	//debug menu stuff
	Key::Key toggleConsole   = Key::TILDE;
	Key::Key toggleDebugMenu = Key::TILDE | InputMod_Lctrl;
	Key::Key toggleDebugBar  = Key::TILDE | InputMod_Lshift;
    
	//main menu bar
	Key::Key toggleMenuBar = Key::TILDE | Key::LALT;
    
	//selected object manipulation modes
	Key::Key grabSelectedObject   = Key::G;
	Key::Key rotateSelectedObject = Key::R;
	Key::Key scaleSelectedObject  = Key::S;
    
	Key::Key undo  = Key::Z | InputMod_Lctrl;
	Key::Key redo  = Key::Y | InputMod_Lctrl;
	Key::Key cut   = Key::X | InputMod_Lctrl;
	Key::Key copy  = Key::C | InputMod_Lctrl;
	Key::Key paste = Key::V | InputMod_Lctrl;
    
    ConfigMap keybindMap;
    
	void Init();
	void Update();
    void Save();
    void Load();
};

#endif //ATMOS_CONTROLLER_H