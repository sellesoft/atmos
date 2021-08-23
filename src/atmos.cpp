/*atmos

Game engine built on the 3D graphics engine deshi

Major Ungrouped TODOs
---------------------
fully reimplement admin
reimplement editor
reimplement several missing attributes (audio, light, movement, player, etc)
reimplement physics system

Minor Ungrouped TODOs
---------------------
none for now

Admin TODOs
-----------


Attribute TODOs
---------------


Entity TODOs
------------


Controller TODOs
----------------


CameraInstance TODOs
--------------------


Canvas TODOs
------------


Fun TODOs
---------



Bug Board
---------
(08/22/21) debug bar's FPS is 10x less than it should be

*/




#include "deshi.h"
#include "Admin.h"
#include "core/console.h"

local Admin admin; Admin* g_admin = &admin;

int main() {
    //init deshi
	deshi::init();
    
    //init atmos
	AtmoAdmin->Init();
    
	TIMER_START(t_d); TIMER_START(t_f);
	while (!deshi::shouldClose()) {
		DeshiImGui::NewFrame();                    //place imgui calls after this
		DeshTime->   Update();
		DeshWindow-> Update();
	    DeshInput->  Update();
		AtmoAdmin->  Update();
		DeshConsole->Update(); Console2::Update();
		Render::Update();                          //place imgui calls before this
		AtmoAdmin->PostRenderUpdate();
        
		DeshTime->frameTime = TIMER_END(t_f); TIMER_RESET(t_f);
	}
    
	deshi::cleanup();
	AtmoAdmin->Cleanup();
}