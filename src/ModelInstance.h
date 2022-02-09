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
	bool visible;
	
	ModelInstance();
	ModelInstance(Model* model);
	ModelInstance(Mesh* mesh);
	~ModelInstance();
	
	void Update();
	void ChangeModel(Model* model);
	void ChangeModel(Mesh* mesh);
	inline void ToggleVisibility(){ visible = !visible; }
	
	static void SaveText(ModelInstance* model, string& level);
	//static void LoadText();
};

#endif //ATMOS_MODELINSTANCE_H