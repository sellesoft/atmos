#include "Admin.h"

#include "attributes/Attribute.h"
#include "attributes/ModelInstance.h"

#include "core/storage.h"

#include "entities/Entity.h"

void Admin::Init() {
	keybinds.init();
	camera = CameraInstance(90);
	controller.Init();

	editor.Init();

	Entity* test = new Entity();
	test->name = "test";
	ModelInstance* mi = new ModelInstance(Storage::CreateModelFromFile("box.obj").second);
	mi->entity = test;
	test->attributes.add(mi);
	test->transform = Transform(vec3::ZERO, vec3::ZERO, vec3::ONE);
	attributes.add(test->attributes);
	entities.add(test);
}

void Admin::Update() {
	controller.Update();
	camera.Update();
	editor.Update();
	for (Attribute* attr : attributes) {
		attr->Update();
	}
}

void Admin::PostRenderUpdate() {

}

void Admin::Reset() {

}

void Admin::Cleanup() {

}