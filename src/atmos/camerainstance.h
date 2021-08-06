#pragma once
#ifndef COMPONENT_CAMERA_H
#define COMPONENT_CAMERA_H

#include "core/camera.h"

#include "math/vec.h"
#include "math/mat.h"

struct CameraInstance : public Camera {
	CameraInstance() {};
	CameraInstance(float fov, float nearZ = .01f, float farZ = 1000.01f, bool freeCam = true);

	void Update();

	mat4 MakePerspectiveProjection();
	mat4 MakeOrthographicProjection();

	void UpdateProjectionMatrix();
};

#endif //COMPONENT_CAMERA_H