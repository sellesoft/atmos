#include "controller.h"
#include "Admin.h"
#include "camerainstance.h"
#include "core/input.h"
#include "core/window.h"

#include <cstring>

local f32 MOUSE_SENS_FRACTION = .03f;

void Controller::Init() {
	camera = &AtmoAdmin->camera;
	mouseSens = 2.5;
}

void Controller::Update() {
	//camera movement
	float dTime = DeshTime->deltaTime;

	vec3 inputs = vec3::ZERO;

	if (DeshInput->KeyDownAnyMod(MouseButton::RIGHT)) {
		if (DeshInput->KeyDownAnyMod(AtmoKeys.movementFlyingUp))      { inputs.y += 1; }
		if (DeshInput->KeyDownAnyMod(AtmoKeys.movementFlyingDown))    { inputs.y -= 1; }
		if (DeshInput->KeyDownAnyMod(AtmoKeys.movementFlyingForward)) { inputs += camera->forward; }
		if (DeshInput->KeyDownAnyMod(AtmoKeys.movementFlyingBack))    { inputs -= camera->forward; }
		if (DeshInput->KeyDownAnyMod(AtmoKeys.movementFlyingRight))   { inputs += camera->right; }
		if (DeshInput->KeyDownAnyMod(AtmoKeys.movementFlyingLeft))    { inputs -= camera->right; }

		if      (DeshInput->LShiftDown()){ camera->position += inputs.normalized() * 16 * dTime; }
		else if (DeshInput->LCtrlDown()) { camera->position += inputs.normalized() * 4 * dTime; }
		else                             { camera->position += inputs.normalized() * 8 * dTime; }
	}

	//camera rotation
	if (DeshInput->KeyPressedAnyMod(MouseButton::RIGHT))
		DeshWindow->UpdateCursorMode(CursorMode_FirstPerson);

	if (DeshInput->KeyReleasedAnyMod(MouseButton::RIGHT))
		DeshWindow->UpdateCursorMode(CursorMode_Default);

	if(DeshInput->KeyDownAnyMod(MouseButton::RIGHT)){
		camera->rotation.y += (DeshInput->mouseX - DeshWindow->centerX) * mouseSens * MOUSE_SENS_FRACTION;
		camera->rotation.x += (DeshInput->mouseY - DeshWindow->centerY) * mouseSens * MOUSE_SENS_FRACTION;
	}
}