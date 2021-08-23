#include "Admin.h"
#include "attributes/Attribute.h"
#include "attributes/ModelInstance.h"
#include "entities/Entity.h"
#include "core/storage.h"

void Admin::Init() {
	camera = CameraInstance(90);
	controller.Init();
	editor.Init();
    
    {//atmos sandbox
        Entity* test = new Entity();
        test->name = "test2";
        ModelInstance* mi = new ModelInstance(Storage::CreateModelFromFile("box.obj").second);
        mi->entity = test;
        test->attributes.add(mi);
        test->transform = Transform{vec3::ZERO, vec3::ZERO, vec3::ONE};
        attributes.add(test->attributes);
        entities.add(test);
    }
}

void Admin::Update(){
	controller.Update();
	camera.Update();
	editor.Update();
    
	for(Attribute* attr : attributes){
		attr->Update();
	}
}

void Admin::PostRenderUpdate(){
    
}

void Admin::Reset(){
    
}

void Admin::Cleanup(){
    
}