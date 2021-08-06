#include "controller.h"
#include "camerainstance.h"
#include "core/input.h"
#include "core/window.h"
#include "Admin.h"

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
		if (DeshInput->KeyDownAnyMod(AtmoBinds.movementFlyingUp))      { inputs.y += 1; }
		if (DeshInput->KeyDownAnyMod(AtmoBinds.movementFlyingDown))    { inputs.y -= 1; }
		if (DeshInput->KeyDownAnyMod(AtmoBinds.movementFlyingForward)) { inputs += camera->forward; }
		if (DeshInput->KeyDownAnyMod(AtmoBinds.movementFlyingBack))    { inputs -= camera->forward; }
		if (DeshInput->KeyDownAnyMod(AtmoBinds.movementFlyingRight))   { inputs += camera->right; }
		if (DeshInput->KeyDownAnyMod(AtmoBinds.movementFlyingLeft))    { inputs -= camera->right; }

		if      (DeshInput->LShiftDown()){ camera->position += inputs.normalized() * 16 * dTime; }
		else if (DeshInput->LCtrlDown()) { camera->position += inputs.normalized() * 4 * dTime; }
		else                             { camera->position += inputs.normalized() * 8 * dTime; }
	}

	//camera rotation
	if (DeshInput->KeyPressedAnyMod(MouseButton::RIGHT))
		DeshWindow->UpdateCursorMode(CursorMode::FIRSTPERSON);

	if (DeshInput->KeyReleasedAnyMod(MouseButton::RIGHT))
		DeshWindow->UpdateCursorMode(CursorMode::DEFAULT);

	if(DeshInput->KeyDownAnyMod(MouseButton::RIGHT)){
		camera->rotation.y += (DeshInput->mouseX - DeshWindow->centerX) * mouseSens * MOUSE_SENS_FRACTION;
		camera->rotation.x += (DeshInput->mouseY - DeshWindow->centerY) * mouseSens * MOUSE_SENS_FRACTION;
	}
}