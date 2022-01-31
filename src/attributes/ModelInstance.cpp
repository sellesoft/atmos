#include "ModelInstance.h"
#include "../entities/Entity.h"
#include "core/renderer.h"
#include "core/storage.h"
#include "utils/string_utils.h"

ModelInstance::ModelInstance(){
	model    = Storage::NullModel();
	mesh     = model->mesh;
	armature = model->armature;
	visible  = true;
}

ModelInstance::ModelInstance(Model* _model){
	model    = _model;
	mesh     = model->mesh;
	armature = model->armature;
	visible  = true;
}

ModelInstance::ModelInstance(Mesh* _mesh){
	model    = Storage::CreateModelFromMesh(_mesh).second;
	mesh     = model->mesh;
	armature = model->armature;
	visible  = true;
}

ModelInstance::~ModelInstance(){
	//Storage::DeleteModel(_model); //TODO re-enable once Storage::DeleteModel() is setup
}

void ModelInstance::ChangeModel(Model* _model){
	//Storage::DeleteModel(_model); //TODO re-enable once Storage::DeleteModel() is setup
	model    = _model;
	mesh     = model->mesh;
	armature = model->armature;
}

void ModelInstance::ChangeModel(Mesh* _mesh){
	//Storage::DeleteModel(_model); //TODO re-enable once Storage::DeleteModel() is setup
	model    = Storage::CreateModelFromMesh(_mesh).second;
	mesh     = model->mesh;
	armature = model->armature;
}

void ModelInstance::Update(){
	if(visible) Render::DrawModel(model, attribute.entity->transform.Matrix());
}

void ModelInstance::SaveText(ModelInstance* m, string& level){
	Storage::SaveModel(m->model);
	level += toStr("\n:",AttributeType_ModelInstance," #",AttributeTypeStrings[AttributeType_ModelInstance],
				   "\nmodel   \"",m->model->name,"\""
				   "\nvisible ",(m->visible) ? "true" : "false");
}