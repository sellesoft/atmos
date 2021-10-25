#pragma once
#ifndef ATMOS_CAMERAINSTANCE_H
#define ATMOS_CAMERAINSTANCE_H

#include "core/camera.h"
#include "math/math.h"

//TODO(delle) make this an attribute
struct CameraInstance : public Camera{
	CameraInstance() {};
	CameraInstance(float fov, float nearZ = .01f, float farZ = 10000.01f, bool freeCam = true);
	
	void Update();
	
	mat4 MakePerspectiveProjection();
	mat4 MakeOrthographicProjection();
	
	void UpdateProjectionMatrix();
};

#endif //ATMOS_CAMERAINSTANCE_H