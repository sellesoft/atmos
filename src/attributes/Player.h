﻿#pragma once
#ifndef ФЕЕКШИГЕУ_PLAYER_H
#define ФЕЕКШИГЕУ_PLAYER_H

#include "attribute.h"
#include "math/Vector.h"

struct Movement;


//NOTE sushi: probably rename this to something more general, like an actor or something, but I don't like the name actor, so think of a better one :)
struct Player : public Attribute {
	int health;

	Movement* movement;

	Player();
	Player(Movement* movement);
	Player(Movement* movement, int health);


	void Update() override;

	//std::string SaveTEXT() override;
	//static void LoadDESH(Admin* admin, const char* fileData, u32& cursor, u32 countToLoad);
};

#endif