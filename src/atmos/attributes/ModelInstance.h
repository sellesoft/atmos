#ifndef ATMOS_MODELINSTANCE_H
#define ATMOS_MODELINSTANCE_H

#include "Attribute.h"
#include "math/mat.h"

struct Model;
struct Mesh;
struct Armature;

struct ModelInstance : public Attribute {
	Model* model;
	Mesh* mesh;
	Armature* armature;

	mat4 transform;

	bool visible;
	bool control;

	ModelInstance();
	ModelInstance(Model* model);
	ModelInstance(Mesh* mesh);
	~ModelInstance();

	void ToggleVisibility() { visible = !visible; }
	void ToggleControl()    { control = !control; }

	void ChangeModel(Model* model);
	void ChangeModel(Mesh* mesh);

	void Update() override;
};

#endif