#pragma once
#ifndef ATMOS_MODELINSTANCE_H
#define ATMOS_MODELINSTANCE_H

#include "Attribute.h"
#include "math/matrix.h"

struct Model;
struct Mesh;
struct Armature;
struct ModelInstance {
	Attribute attribute{ AttributeType_ModelInstance };
	
	Model* model;
	Mesh* mesh;
	Armature* armature;
	mat4 transform;
	bool visible;
	bool control;
    
	ModelInstance();
	ModelInstance(Model* model);
	ModelInstance(Mesh* mesh);
    
	void Update();
	void ChangeModel(Model* model);
	void ChangeModel(Mesh* mesh);
    inline void ToggleVisibility(){ visible = !visible; }
	inline void ToggleControl()   { control = !control; }
};

#endif //ATMOS_MODELINSTANCE_H