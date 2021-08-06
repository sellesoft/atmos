#include "camerainstance.h"

#include "core/window.h"
#include "core/renderer.h"
#include "core/console.h"
#include "math/Math.h"
#include "utils/debug.h"

CameraInstance::CameraInstance(float fov, float nearZ, float farZ, bool freeCam) {
	this->nearZ = nearZ;
	this->farZ = farZ;
	this->fov = fov;
	this->freeCamera = freeCam;

	position = { 4.f,   3.f,-4.f };
	rotation = { 28.f, -45.f, 0.f };

	this->forward = (vec3::FORWARD * mat4::RotationMatrix(rotation)).normalized();
	this->right = vec3::UP.cross(forward).normalized();
	this->up = right.cross(forward).normalized();
	this->viewMat = Math::LookAtMatrix(position, position + forward).Inverse();
	UpdateProjectionMatrix();
}

void CameraInstance::Update() {
	if (freeCamera) {
		//NOTE this can happen whether the camera is free or not so move it out
		//of this scope once we implement that
		persist int wwidth = DengWindow->width;
		persist int wheight = DengWindow->height;

		//clamp camera crotation
		Math::clamp(rotation.x, -89.9f, 89.9f);
		if (rotation.y > 1440.f || rotation.y < -1440.f) { rotation.y = 0.f; }

		//update direction vectors
		forward = (vec3::FORWARD * mat4::RotationMatrix(rotation)).normalized();
		right = vec3::UP.cross(forward).normalized();
		up = right.cross(forward).normalized();

		viewMat = Math::LookAtMatrix(position, position + forward).Inverse();

		//update renderer camera properties
		if (mode == CameraMode_Orthographic) {
			switch (orthoview) {
			case OrthoViews::FRONT:    position = vec3(0, 0, -999); rotation = vec3(0, 0, 0);     break;
			case OrthoViews::BACK:     position = vec3(0, 0, 999);  rotation = vec3(0, 180, 0);   break;
			case OrthoViews::RIGHT:    position = vec3(999, 0, 0);  rotation = vec3(0, -90, 0);   break;
			case OrthoViews::LEFT:     position = vec3(-999, 0, 0); rotation = vec3(0, 90, 0);    break;
			case OrthoViews::TOPDOWN:  position = vec3(0, 999, 0);  rotation = vec3(89.9, 0, 0);  break;
			case OrthoViews::BOTTOMUP: position = vec3(0, -999, 0); rotation = vec3(-89.9, 0, 0); break;
			}
			UpdateProjectionMatrix();
		}

		//redo projection matrix is window size changes
		if (DengWindow->width != wwidth || DengWindow->height != wheight) {
			wwidth = DengWindow->width;
			wheight = DengWindow->height;
			UpdateProjectionMatrix();
		}

		Render::UpdateCameraViewMatrix(viewMat);
		Render::UpdateCameraPosition(position);

	}
}

mat4 CameraInstance::MakePerspectiveProjection() {
	return Camera::MakePerspectiveProjectionMatrix(DengWindow->width, DengWindow->height, fov, farZ, nearZ);
}

mat4 CameraInstance::MakeOrthographicProjection() {
	//convert bounding box to camera space
	//persist float zoom = 10;
	//vec3 maxcam = Math::WorldToCamera3(vec3(zoom, zoom, zoom), DengAdmin->mainCamera->viewMat);
	//vec3 mincam = Math::WorldToCamera3(vec3(-zoom, -zoom, -zoom), DengAdmin->mainCamera->viewMat);
	//
	////make screen box from camera space bounding box
	//float maxx = std::max(fabs(mincam.x), fabs(maxcam.x));
	//float maxy = std::max(fabs(mincam.y), fabs(maxcam.y));
	//float max = std::max(maxx, maxy);
	//
	//float aspectRatio = (float)DengWindow->width / DengWindow->height;
	//float r = max * aspectRatio, t = max;
	//float l = -r, b = -t;
	//
	//persist float oloffsetx = 0;
	//persist float oloffsety = 0;
	//persist float offsetx = 0;
	//persist float offsety = 0;
	//persist vec2 initmouse;
	//persist bool initoffset = false;
	//
	//PRINTLN(zoom);
	////orthographic view controls
	//if (DengInput->KeyPressed(DengKeys.orthoZoomIn) && zoom > 0.0000000009) zoom -= zoom / 5;
	//if (DengInput->KeyPressed(DengKeys.orthoZoomOut)) zoom += zoom / 5;
	//
	//if (DengInput->KeyPressed(DengKeys.orthoOffset)) initoffset = true;
	//
	//if (DengInput->KeyDown(DengKeys.orthoOffset)) {
	//	if (initoffset) {
	//		initmouse = DengInput->mousePos;
	//		initoffset = false;
	//	}
	//	offsetx = 0.0002 * zoom * (DengInput->mousePos.x - initmouse.x);
	//	offsety = 0.0002 * zoom * (DengInput->mousePos.y - initmouse.y);
	//}
	//
	//if (DengInput->KeyReleased(DengKeys.orthoOffset)) {
	//	oloffsetx += offsetx; oloffsety += offsety;
	//	offsetx = 0; offsety = 0;
	//}
	//
	//if (DengInput->KeyPressed(DengKeys.orthoResetOffset)) {
	//	oloffsetx = 0; oloffsety = 0;
	//}
	//
	//r += zoom * aspectRatio; l = -r;
	//r -= offsetx + oloffsetx; l -= offsetx + oloffsetx;
	//t += zoom; b -= zoom;
	//t += offsety + oloffsety; b += offsety + oloffsety;
	//
	//return Camera::MakeOrthographicProjection(DengWindow->width, DengWindow->height, r, l, t, b, farZ, nearZ);
	return mat4::IDENTITY;
}

void CameraInstance::UpdateProjectionMatrix() {
	switch (mode) {
	case(CameraMode_Perspective):default: { projMat = MakePerspectiveProjection(); } break;
	case(CameraMode_Orthographic):        { projMat = MakeOrthographicProjection(); }break;
	}
	Render::UpdateCameraProjectionMatrix(projMat);
}