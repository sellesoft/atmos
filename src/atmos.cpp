/* atmos: Physics-Puzzle game built in the 3D graphics engine deshi

Admin TODOs
-----------
reimplement timers

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
safety check on renaming things so no things have same name
fix mesh viewing bools mixing?
fix extra temp vertexes/indexes (when viewing triangle neighbors)
fix normals viewing on big objects crash https://stackoverflow.com/questions/63092926/what-is-causing-vk-error-device-lost-when-calling-vkqueuesubmit
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
implement grabbing/rotating/scaling with a visual tool thing (like in Unreal)
orthographic side views
(maybe) multiple viewports
implement orthographic grabbing 
entity filtering in entity list

 Physics TODOs
-------------
make collider trigger latching a boolean, so it can continuous trigger an event while an obj is in it
figure out how to handle multi events being set for a collider. for other components its not really an issue
___because u just choose what event to send at run time, but i don't want to do this inside of the collider
___functions themselves, as it would be way too much clutter. maybe a helper function on collider to choose what to do 
redo main physics loop
add physics collision sweeping
add physics based collision resolution for remaining collider primitives
add physics interaction functions
implement collision manifold generation
implement Complex Colliders

Ungrouped TODOs
---------------
fully reimplement admin
reimplement editor
reimplement several missing attributes (audio, light, player, etc)
reimplement physics system
create a demo level
rework and simplify entity creation so there is a distinction between development and gameplay creation
change entity and admin LoadTEXT to be character based rather than std::string based
think of a way to implement different events being sent between comps as right now it's only one
____is this necessary though? we can define these events at run time, but the connections must be made through UI
____so maybe have a UI option that allows the comps update function to handle it and only connects them.
____actually having an option for anything other than collider is kind of useless soooo maybe 
____get rid of event on every component or just only let u choose that event on colliders
pool/arena components and entities for better performance
binary file saving/loading

Bug Board       //NOTE mark these with a last-known active date (M/D/Y)
---------
(06/13/21) rotating using R no longer seems to work, it wildly rotates the object
__________ it might have something to do with our rotate by axis function
(07/20/21) copy/paste produces an extra mesh in the renderer sometimes

*/

#include "deshi.h"
#include "Admin.h"

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