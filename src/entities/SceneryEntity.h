#pragma once
#ifndef ATMOS_SCENERYENTITY_H
#define ATMOS_SCENERYENTITY_H

#include "Entity.h"
#include "attributes/ModelInstance.h"
#include "core/storage.h"

//mesh that the player should never interact with but can see
struct SceneryEntity : public Entity {
	void Init(const char* _name, Transform _transform, Model* _model) {
		type = EntityType_Scenery;
		name = _name;
		transform = _transform;
		
		Assert(_model);
		AtmoAdmin->modelArr.add(ModelInstance(_model));
		model = AtmoAdmin->modelArr.last;
		
		id = AtmoAdmin->entities.count;
		AtmoAdmin->entities.add(this);
	}
};

#endif //ATMOS_SCENERYENTITY_H