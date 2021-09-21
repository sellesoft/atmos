#pragma once
#ifndef ATMOS_ADMIN_H
#define ATMOS_ADMIN_H

#include "controller.h"
#include "camerainstance.h"
#include "editor.h"
#include "physicssystem.h"
#include "attributes/Movement.h"
#include "attributes/ModelInstance.h"
#include "attributes/Physics.h"
#include "attributes/InterpTransform.h"
#include "utils/array.h"
#include "utils/string.h"
#include "utils/cstring.h"

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
	
    GameState      state;
	Controller     controller;
	CameraInstance camera; //admin controls all cameras in the world, but for now its just one
    PhysicsSystem  physics;
	
    Editor editor;
    b32    simulateInEditor;
    
	PlayerEntity* player; //store player separate so we can access it directly
	array<Entity*> entities;
	array<TriggerEntity*> triggers;
    
	array<Physics> physicsArr;
	array<ModelInstance> modelArr;
	array<InterpTransform> interpTransformArr;
    
	void Init(string _dataPath);
	void Update();
	void PostRenderUpdate();
	void Reset();
	void Cleanup();
    
	void ChangeState(GameState new_state);
	void SaveLevel(cstring level_name);
	void LoadLevel(cstring level_name);
	Entity* EntityRaycast(vec3 origin, vec3 direction, f32 maxDistance, EntityType filter = 0);
};

extern Admin* g_admin;
#define AtmoAdmin g_admin

#endif //ATMOS_ADMIN_H