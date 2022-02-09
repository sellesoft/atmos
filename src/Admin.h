#pragma once
#ifndef ATMOS_ADMIN_H
#define ATMOS_ADMIN_H

#include "controller.h"
#include "camerainstance.h"
#include "editor.h"
#include "physicssystem.h"
#include "Movement.h"
#include "ModelInstance.h"
#include "Physics.h"
#include "InterpTransform.h"
#include "kigu/array.h"
#include "kigu/cstring.h"
#include "kigu/string.h"

enum GameState_{
	GameState_Play,
	GameState_Menu,
	GameState_Editor,
	GameState_COUNT,
}; typedef u32 GameState;

struct Entity;
struct PlayerEntity;
struct TriggerEntity;
typedef u32 EntityType;
struct Admin{
	string dataPath;
	string levelName;
	
	GameState      state;
	Controller     controller;
	CameraInstance camera; //admin controls all cameras in the world, but for now its just one
	PhysicsSystem  physics;
	
	Editor editor;
	b32    simulateInEditor;
	
	b32 loadNextLevel = false;
	u32 levelListIdx = 0;
	cstring levelList[3] = { cstr_lit("drop0"), cstr_lit("drop1"), cstr_lit("drop2"), };
	
	PlayerEntity* player = 0; //store player separate so we can access it directly
	array<Entity*> entities;
	array<TriggerEntity*> triggers;
	
	array<Physics> physicsArr;
	array<ModelInstance> modelArr;
	array<InterpTransform> interpTransformArr;
	
	void Init();
	void Update();
	void PostRenderUpdate();
	void Reset();
	void Cleanup();
	
	void ChangeState(GameState new_state);
	void SaveLevel(cstring level_name);
	void LoadLevel(cstring level_name);
	Entity* EntityRaycast(vec3 origin, vec3 direction, f32 maxDistance, EntityType filter = 0, b32 requireCollider = false);
};

extern Admin* g_admin;
#define AtmoAdmin g_admin

#endif //ATMOS_ADMIN_H