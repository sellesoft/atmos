#pragma once
#ifndef ATMOS_PLAYER_H
#define ATMOS_PLAYER_H

#include "attribute.h"
#include "math/Vector.h"

struct Player {
	Attribute attribute{ AttributeType_Player };
	
	int health;
	
	Player(){}
	Player(int _health) : health(_health){}
	
	void Update(){}
	
	//std::string SaveTEXT() override;
	//static void LoadDESH(Admin* admin, const char* fileData, u32& cursor, u32 countToLoad);
};

#endif //ATMOS_PLAYER_H