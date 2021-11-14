/* atmos: Physics-Puzzle game built in the 3D graphics engine deshi

Admin TODOs
-----------
reimplement timers
save gravity with levels

Attribute TODOs
---------------
once we reimplement saving/loading, stop using big ass constructors to load everything into an object and just use
____{} or something

Entity TODOs
------------

Controller TODOs
----------------

 Editor TODOs
------------
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
fully reimplement admin
reimplement editor
reimplement several missing attributes (audio, light, player, etc)
reimplement physics system
create a demo level
rework and simplify entity creation so there is a distinction between development and gameplay creation
change entity and admin LoadTEXT to be character based rather than std::string based
pool/arena components and entities for better performance
binary file saving/loading

Bug Board       //NOTE mark these with a last-known active date (M/D/Y)
---------
(10/18/21) entities renamed in the editor often get their name scuffed when saving them

*/

#include "deshi.h"
#include "core/commands.h"

#define ATMOS_IMPLEMENTATION
#include "Admin.h"
#include "menu.h"
#include "atmos_commands.cpp"
//#include "menu.cpp"

local Admin admin; Admin* g_admin = &admin;

int main() {
	//init deshi
	deshi::init();
	
	/*
	//SSE testing
	{
		__m128 var0 = FillSSE(5);                //var0 = 5, 5, 5, 5
		__m128 var1 = FillSSE(-5.0f);            //var1 = -5, -5, -5, -5
		var0 = NegateSSE(var0);                  //var0 = -5, -5, -5, -5
		b32 test0 = EpsilonEqualSSE(var0, var1); //test0 = true
		Assert(test0);
		var0 = AbsoluteSSE(var0);                //var0 = 5, 5, 5, 5
		vec4 var2{2, 2, 2, 2};                   //var2 = 2, 2, 2, 2
		vec4 var3(var2);                         //var3 = 2, 2, 2, 2
		vec4 var4(&var2.x);                      //var4 = 2, 2, 2, 2
		b32 test1 = (var2 == var3);              //test1 = true
		Assert(test0);
		var3.set(5, 5, 5, 5);                    //var3 = 5, 5, 5, 5
		var3 *= 2.0f;                            //var3 = 10, 10, 10, 10
		var4 = var3 / var2;                      //var4 = 5, 5, 5, 5
		var4 -= vec4{1, 1, 1, 1};                //var4 = 4, 4, 4, 4
		var4 = -var4;                            //var4 = -4, -4, -4, -4
		var4 = var4.absV();                      //var4 = 4, 4, 4, 4
		f32 var5 = var4.dot(var4);               //var5 = 64
		Assert(EpsilonEqual(var5, 64.0f));
		f32 var6 = Sqrt(var5);                   //var6 = 8
		Assert(EpsilonEqual(var6, 8.0f));
		var6 = Rsqrt(var5);                      //var6 = 0.125
		Assert(abs(var6 - 0.125f) < 10*M_EPSILON);
	}
	*/
	
	//init atmos
	AddAtmosCommands();
	AtmoAdmin->Init();
	
	TIMER_START(t_d); TIMER_START(t_f);
	while (!deshi::shouldClose()) {
		DeshiImGui::NewFrame();                    //place imgui calls after this
		DeshTime->   Update();
		DeshWindow-> Update();
	    DeshInput->  Update();
		AtmoAdmin->  Update();
		DeshConsole->Update(); Console2::Update();
		UI::         Update();
		Render::     Update();                     //place imgui calls before this
		AtmoAdmin->  PostRenderUpdate();
		
		DeshTime->frameTime = TIMER_END(t_f); TIMER_RESET(t_f);
	}
	
	deshi::cleanup();
	AtmoAdmin->Cleanup();
}