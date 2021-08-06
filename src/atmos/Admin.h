#ifndef ATMOS_ADMIN_H
#define ATMOS_ADMIN_H

#include "keybinds.h"
#include "controller.h"
#include "camerainstance.h"


#include "utils/array.h"
#include "utils/string.h"

struct Entity;
struct Attribute;

struct Admin {

	Keybinds     keybinds;
	CameraInstance camera; //admin controls all cameras in the world, but for now its just one
	Controller controller;

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
#define AtmoBinds g_admin->keybinds

#endif