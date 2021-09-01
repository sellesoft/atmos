#pragma once
#ifndef ФЕЕКШИГЕУ_PLAYER_H
#define ФЕЕКШИГЕУ_PLAYER_H

#include "attribute.h"
#include "math/Vector.h"

struct Player : public Attribute {
	int health;

	Player();
	Player(int health);

	void Update() override;

	//std::string SaveTEXT() override;
	//static void LoadDESH(Admin* admin, const char* fileData, u32& cursor, u32 countToLoad);
};

#endif