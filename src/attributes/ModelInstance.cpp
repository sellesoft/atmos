#include "ModelInstance.h"
#include "core/renderer.h"
#include "core/storage.h"
#include "../entities/Entity.h"

ModelInstance::ModelInstance(){
	type = AttributeType_ModelInstance;
	model     = Storage::NullModel();
	mesh      = model->mesh;
	armature  = model->armature;
	transform = mat4::IDENTITY;
	visible   = true;
	control   = false;
}

ModelInstance::ModelInstance(Model* _model){
	type = AttributeType_ModelInstance;
	model     = _model;
	mesh      = model->mesh;
	armature  = model->armature;
	transform = mat4::IDENTITY;
	visible   = true;
	control   = false;
}

ModelInstance::ModelInstance(Mesh* _mesh){
	type = AttributeType_ModelInstance;
	model     = Storage::CreateModelFromMesh(_mesh).second;
	mesh      = model->mesh;
	armature  = model->armature;
	transform = mat4::IDENTITY;
	visible   = true;
	control   = false;
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
	if(!control) transform = entity->transform.Matrix();
	if(visible)  Render::DrawModel(model, transform);
}