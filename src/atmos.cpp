/* atmos: Physics-Puzzle game built in the 3D graphics engine deshi

Admin TODOs
-----------
reimplement system timers
save gravity with levels
change entity and admin LoadTEXT to be character based rather than std::string based
pool/arena components and entities for better performance
move admin to atmos.cpp

Attribute TODOs
---------------
reimplement several missing attributes (audio, light, player, etc)
once we reimplement saving/loading, stop using big ass constructors to load everything into an object and just use
____{} or something

Entity TODOs
------------
rework and simplify entity creation so there is a distinction between development and gameplay creation
add special transform inverse matrix calculation since it can be alot faster than generalized
____https://lxjk.github.io/2017/09/03/Fast-4x4-Matrix-Inverse-with-SSE-SIMD-Explained.html
rework events

Controller TODOs
----------------

 Editor TODOs
------------
reimplement editor with UI
rework undoing (physics isnt updated by it currently)
safety check on renaming things so no things have same name
fix mesh viewing bools mixing?
fix extra temp vertexes/indexes (when viewing triangle neighbors)
fix normals viewing on big objects crash
 ____https://stackoverflow.com/questions/63092926/what-is-causing-vk-error-device-lost-when-calling-vkqueuesubmit
change undo's to never use pointers and have undos that can act like linked lists to chain them
rewrite the events menu (triggers need to be able to filter what causes them to activate)
add box select for mesh inspector and entity selection
editor settings (world grid, colors, positions)
add safety checks on renaming things so no two meshes/materials/models have the same name
see folders inside different folders when loading things (leading into a asset viewer)
orthographic grabbing/rotating
add transfering the player pointer between entities that have an actor comp (combo in Global Tab)
orbitting camera for rotating around objects
context menu when right clicking on an object 
typing numbers while grabbing/rotating/scaling for precise manipulation (like in Blender)
orthographic side views
(maybe) multiple viewports
entity filtering in entity list
redo debug bar to be more informative and have different modes

 Physics TODOs
-------------
add warm starting
add time of impact alterations
add object sleeping
add simulation regions
setup memory locality (arena the relavent memory)
setup callbacks before resolution and after resolution
test contact point reduction in FaceSAT collisions
store previous frame's manifolds and separating axes and check them first to see if still valid
store physics mass and collider tensor as inverse
optimize detection by removing duplicate calculation of matrices and collider offsets
account for entity scale in sphere collisions
add compound colliders (multiple colliders that represent one physics object)
add convex mesh inertia tensor generation
rework collider to have flags: player only collision, dont collide with others with this flag, dont resolve collisions, 
____dont collide at all (for some reason), trigger, etc
add collision sweeping
add collision resolution for: cylinder, capsule, box
add overall outer vertexes to mesh data
add ability for colliders to modify physics in that space (lower gravity in field, high air friction, etc)

Ungrouped TODOs
---------------
add Transform::ZERO and change usages of default constructor
create a demo level
binary file saving/loading

Bug Board       //NOTE mark these with a last-known active date (M/D/Y)
---------
(10/18/21) entities renamed in the editor often get their name scuffed when saving them
(12/08/21) phys_demo isnt working (physics positions are wrong on load)

*/

//// kigu includes ////
#include "kigu/common.h"
#include "kigu/common.h"

//// deshi includes ////
#include "core/memory.h"
#include "core/assets.h"
#include "core/commands.h"
#include "core/console.h"
#include "core/logger.h"
#include "core/imgui.h"
#include "core/input.h"
#include "core/io.h"
#include "core/renderer.h"
#include "core/storage.h"
#include "core/time.h"
#include "core/ui.h"
#include "core/window.h"
#include "math/geometry.h"

//// atmos files ////
#define ATMOS_IMPLEMENTATION 1
#include "admin.h"
#include "attribute.h"
#include "camerainstance.h"
#include "collider.h"
#include "controller.h"
#include "entity.h"
#include "editor.h"
#include "editor2.h"
#include "interptransform.h"
#include "menu.h"
#include "modelinstance.h"
#include "movement.h"
#include "physics.h"
#include "player.h"
#include "physicssystem.h"
#include "transform.h"

#include "admin.cpp"
#include "atmos_commands.cpp"
#include "camerainstance.cpp"
#include "collider.cpp"
#include "controller.cpp"
#include "editor.cpp"
#include "modelinstance.cpp"
#include "movement.cpp"
#include "physics.cpp"
#include "physicssystem.cpp"

local Admin admin; Admin* g_admin = &admin;

int main(){
	//init deshi
	Assets::enforceDirectories();
	memory_init(Gigabytes(1), Gigabytes(1));
	Logger::Init(5, true);
	DeshConsole->Init();
	DeshTime->Init();
	DeshWindow->Init("deshi", 1280, 720);
	Render::Init();
	DeshiImGui::Init();
	Storage::Init();
	UI::Init();
	Cmd::Init();
	DeshWindow->ShowWindow();
	Render::UseDefaultViewProjMatrix();
	
	//init atmos
	AddAtmosCommands();
	AtmoAdmin->Init();
	
	TIMER_START(t_f);
	while(!DeshWindow->ShouldClose()){
		DeshWindow-> Update();
		DeshiImGui::NewFrame();                    //place imgui calls after this
		DeshTime->   Update();
	    DeshInput->  Update();
		AtmoAdmin->  Update();
		DeshConsole->Update();
		UI::         Update();
		Render::     Update();                     //place imgui calls before this
		AtmoAdmin->  PostRenderUpdate();
		memory_clear_temp();
		DeshTime->frameTime = TIMER_END(t_f); TIMER_RESET(t_f);
	}
	
	//cleanup atmos
	AtmoAdmin->Cleanup();
	
	//cleanup deshi
	DeshiImGui::Cleanup();
	Render::Cleanup();
	DeshWindow->Cleanup();
	Logger::Cleanup();
	memory_cleanup();
}