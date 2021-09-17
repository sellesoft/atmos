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
#include "utils/array.h"
#include "utils/string.h"

enum GameState_{
    GameState_Play,
    GameState_Menu,
    GameState_Editor,
    GameState_COUNT,
}; typedef u32 GameState;

struct Entity;
struct PlayerEntity;
typedef u32 EntityType;
struct Admin{
    GameState      gameState;
	Controller     controller;
	CameraInstance camera; //admin controls all cameras in the world, but for now its just one
    PhysicsSystem  physics;
	
    Editor editor;
    b32    simulateInEditor;
    
	PlayerEntity*  player; //store player separate so we can access it directly
	array<Entity*> entities;
    
	array<Movement>      movementArr;
	array<Physics>       physicsArr;
	array<ModelInstance> modelArr;
    
	void Init();
	void Update();
	void PostRenderUpdate();
	void Reset();
	void Cleanup();
    
	Entity* EntityRaycast(vec3 origin, vec3 direction, f32 maxDistance, EntityType filter = 0);
};

extern Admin* g_admin;
#define AtmoAdmin g_admin

#endif //ATMOS_ADMIN_H