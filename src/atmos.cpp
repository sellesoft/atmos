#include "deshi/deshi.h"
#include "camerainstance.h"

int main() {
	deshi::init();

	CameraInstance camera(90);

	TIMER_START(t_d); TIMER_START(t_f);
	while (!deshi::shouldClose()) {
		
		camera.Update();

		Render::DrawBox(mat4::TransformationMatrix(vec3::ZERO, vec3(0, DengTotalTime * 10, 0), vec3::ONE));

		DeshiImGui::NewFrame();                    //place imgui calls after this
		DengTime->Update();                       
		DengWindow->Update();                     
	    DengInput->Update();                       
		DengConsole->Update(); Console2::Update();
		Render::Update();                          //place imgui calls before this
		
		UI::BeginWindow("test", vec2(300, 300), vec2(300, 300));
		UI::Text("test");
		UI::EndWindow();

		UI::Update();
		

		DengTime->frameTime = TIMER_END(t_f); TIMER_RESET(t_f);

		DengWindow->UpdateTitle(TOSTRING(1 / DengTime->deltaTime).str);
	}

	deshi::cleanup();
}