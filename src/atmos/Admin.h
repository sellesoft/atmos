#ifndef ATMOS_ADMIN_H
#define ATMOS_ADMIN_H

#include "keybinds.h"
#include "controller.h"
#include "camerainstance.h"
#include "Editor.h"


#include "utils/array.h"
#include "utils/string.h"

struct Entity;
struct Attribute;

struct Admin {

	bool paused = 0;
	bool pause_phys = 0;

	Controller controller;
	Keybinds     keybinds;
	CameraInstance camera; //admin controls all cameras in the world, but for now its just one
	Editor         editor;

	array<Entity*> entities;
	array<Attribute*> attributes;

	void Init();
	void Update();
	void PostRenderUpdate();
	void Reset();
	void Cleanup();

};

extern Admin* g_admin;

#define AtmoAdmin g_admin
#define AtmoKeys g_admin->keybinds

#endif