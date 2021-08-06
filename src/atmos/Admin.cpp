#include "Admin.h"

#include "attributes/Attribute.h"
#include "attributes/ModelInstance.h"

#include "core/storage.h"

#include "entities/Entity.h"

void Admin::Init() {
	keybinds.init();
	camera = CameraInstance(90);
	controller.Init();

	Entity* test = new Entity();
	test->name = "test";
	test->attributes.add(new ModelInstance(Storage::CreateModelFromOBJ("box.obj").second));


}

void Admin::Update() {
	controller.Update();
	camera.Update();
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