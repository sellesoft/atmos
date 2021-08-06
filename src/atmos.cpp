#include "deshi/deshi.h"
#include "atmos/camerainstance.h"
#include "atmos/controller.h"
#include "atmos/keybinds.h"

int main() {
	deshi::init();

	CameraInstance camera(90);

	Keybinds keybinds;
	keybinds.init();

	Controller controller;
	controller.camera = &camera;
	controller.keybinds = keybinds;
	controller.mouseSens = 2.5;

	//Render::UseDefaultViewProjMatrix(vec3{ 4.f, 3.f,-4.f }, vec3{ 28.f, -45.f, 0.f });

	TIMER_START(t_d); TIMER_START(t_f);
	while (!deshi::shouldClose()) {
		
		controller.Update();
		camera.Update();

		Render::DrawBox(mat4::TransformationMatrix(vec3::ZERO, vec3(0, DeshTotalTime * 10, 0), vec3::ONE));

		DeshiImGui::NewFrame();                    //place imgui calls after this
		DeshTime->Update();                       
		DeshWindow->Update();                     
	    DeshInput->Update();                       
		DeshConsole->Update(); Console2::Update();
		Render::Update();                          //place imgui calls before this
		

		DeshTime->frameTime = TIMER_END(t_f); TIMER_RESET(t_f);
	}

	deshi::cleanup();
}