#pragma once
#ifndef ATMOS_CONTROLLER_H
#define ATMOS_CONTROLLER_H

#include "defines.h"
#include "core/input.h"
#include "core/assets.h"

struct CameraInstance;
struct Controller{
	//// game ////
	
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
	
	//// editor ////
	
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
	
	//editor UI
	Key::Key toggleConsole   = Key::TILDE | InputMod_AnyCtrl;
	Key::Key toggleDebugMenu = Key::TILDE | InputMod_AnyCtrl;
	Key::Key toggleDebugBar  = Key::TILDE | InputMod_AnyShift;
	Key::Key toggleMenuBar   = Key::TILDE | InputMod_AnyAlt;
	
	//selected object manipulation modes
	Key::Key grabSelectedObject   = Key::G;
	Key::Key rotateSelectedObject = Key::R;
	Key::Key scaleSelectedObject  = Key::S;
	
	Key::Key undo  = Key::Z | InputMod_AnyCtrl;
	Key::Key redo  = Key::Y | InputMod_AnyCtrl;
	Key::Key cut   = Key::X | InputMod_AnyCtrl;
	Key::Key copy  = Key::C | InputMod_AnyCtrl;
	Key::Key paste = Key::V | InputMod_AnyCtrl;
	
	Key::Key physicsPause       = Key::SPACE | InputMod_None;
	Key::Key physicsStep        = Key::SPACE | InputMod_AnyCtrl;
	Key::Key physicsSolving     = Key::SPACE | InputMod_AnyShift;
	Key::Key physicsIntegrating = Key::SPACE | InputMod_AnyAlt;
	Key::Key physicsEditorSim   = Key::SPACE | InputMod_AnyCtrl | InputMod_AnyShift | InputMod_AnyAlt;
	
	Key::Key transformTranslate = Key::T;
	Key::Key transformRotate    = Key::R;
	Key::Key transformScale     = Key::S;
	
	Key::Key saveLevel = Key::S | InputMod_AnyCtrl;
	
	ConfigMap keybindMap;
	
	void Init();
	void Update();
	void Save();
	void Load();
};

#endif //ATMOS_CONTROLLER_H