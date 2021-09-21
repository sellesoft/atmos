#include "ModelInstance.h"
#include "core/renderer.h"
#include "core/storage.h"
#include "../entities/Entity.h"

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

void ModelInstance::ChangeModel(Model* _model){
	model    = _model;
	mesh     = model->mesh;
	armature = model->armature;
}

void ModelInstance::ChangeModel(Mesh* _mesh){
	model    = Storage::CreateModelFromMesh(_mesh).second;
	mesh     = model->mesh;
	armature = model->armature;
}

void ModelInstance::Update(){
	if(visible) Render::DrawModel(model, attribute.entity->transform.Matrix());
}