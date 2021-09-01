#pragma once
#ifndef ATMOS_ADMIN_H
#define ATMOS_ADMIN_H

#include "controller.h"
#include "camerainstance.h"
#include "Editor.h"
#include "utils/array.h"
#include "utils/string.h"

#include "attributes/Attribute.h"
#include "attributes/Collider.h"
#include "attributes/Movement.h"
#include "attributes/Player.h"
#include "attributes/ModelInstance.h"
#include "attributes/Physics.h"

struct Entity;
struct PlayerEntity;
struct Attribute;

typedef u32 EntityType;

struct Admin{
	bool paused = 0;
	bool pause_phys = 0;
    
	Controller controller;
	CameraInstance camera; //admin controls all cameras in the world, but for now its just one
	Editor         editor;
    
	//we store player separate from the rest so we can do special cases with it 
	PlayerEntity* player;
	array<Entity*> entities;

	array<Movement>      movementArr;
	array<Collider>      colliderArr;
	array<ModelInstance> modelArr;
	array<Physics>       physicsArr;


	void Init();
	void Update();
	void PostRenderUpdate();
	void Reset();
	void Cleanup();

	//i dont know if these functions are relevant anymore, since now we are dealing with static entities
	//and you can just make them in place yourself
	u32 CreateEntity(const char* name = 0);
	u32 CreateEntity(Entity* entity);

	void DeleteEntity(u32 id);
	void DeleteEntity(Entity* entity);

	Entity* EntityRaycast(vec3 origin, vec3 direction, f32 maxDistance, EntityType filter = 0);
};

extern Admin* g_admin;
#define AtmoAdmin g_admin

#endif //ATMOS_ADMIN_H