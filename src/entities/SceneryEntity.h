#pragma once
#ifndef ATMOS_SCENERYENTITY_H
#define ATMOS_SCENERYENTITY_H

#include "Entity.h"
#include "attributes/ModelInstance.h"

//mesh that the player should never interact with but can see
struct SceneryEntity : public Entity {
	ModelInstance* model;
	
	void Init(const char* _name, Transform _transform, Model* _model) {
		type = EntityType_Scenery;
		name = _name;
		transform = _transform;
		
		Assert(_model);
		model = ModelInstance(_model); modelPtr = &model;
		AtmoAdmin->entities.add(this);
	}
};


#endif //ATMOS_SCENERYENTITY_H