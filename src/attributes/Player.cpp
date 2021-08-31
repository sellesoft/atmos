#include "Player.h"
//#include "Physics.h"
#include "Movement.h"
#include "../admin.h"

Player::Player() {
	type = AttributeType_Player;
}

Player::Player(Movement* movement) {
	type = AttributeType_Player;
	this->movement = movement;
}

Player::Player(Movement* movement, int health) {
	type = AttributeType_Player;
	this->movement = movement;
	this->health = health;

}

void Player::Update() {

}

//std::string Player::SaveTEXT() {
//	return TOSTRING("\n>player"
//		"\nhealth ", health,
//		"\n");
//}
//
//void Player::LoadDESH(Admin* admin, const char* data, u32& cursor, u32 count) {
//	ERROR_LOC("LoadDESH not setup");
//}