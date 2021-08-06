#pragma once

#ifndef ATMOS_CONTROLLER_H
#define ATMOS_CONTROLLER_H

#include "defines.h"
#include "keybinds.h"

struct CameraInstance;

struct Controller {
	CameraInstance* camera;
	Keybinds keybinds;

	f32 mouseSens;

	void Update();

};




#endif