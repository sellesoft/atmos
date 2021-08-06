#ifndef ATMOS_EDITOR_H
#define ATMOS_EDITOR_H

#include "defines.h"
#include "utils/Color.h"
#include "utils/array.h"
#include "utils/string.h"
#include "math/vec.h"

struct Admin;
struct Entity;
struct CameraInstance;

struct Editor {
	Admin* admin;
	//EditorSettings settings;

	array<Entity*> selected;

	CameraInstance* camera;

	//UndoManager undoer;

	string level_name;
	
	bool popoutInspector;

	bool showInspector;
	bool showTimes;
	bool showDebugBar;
	bool showMenuBar;
	bool showImGuiDemoWindow;
	bool showDebugLayer;
	bool showWorldGrid;
	bool ConsoleHovFlag; //this can be done better

	void Init();
	void Update();
	void Reset();
	void Cleanup();

	void MenuBar();
	void DebugLayer();
	void DebugBar();
	void Inspector();
	void DrawTimes();
	void WorldGrid(vec3 cpos);
	void ShowWorldAxis();
	void ShowSelectedEntityNormals();

};

namespace ImGui {
	void BeginDebugLayer();
	void EndDebugLayer();
	void DebugDrawCircle(vec2 pos, float radius, Color color = Color::WHITE);
	void DebugDrawCircle3(vec3 pos, float radius, Color color = Color::WHITE);
	void DebugDrawCircleFilled3(vec3 pos, float radius, Color color = Color::WHITE);
	void DebugDrawLine(vec2 pos1, vec2 pos2, Color color = Color::WHITE);
	void DebugDrawLine3(vec3 pos1, vec3 pos2, Color color = Color::WHITE);
	void DebugDrawText(const char* text, vec2 pos, Color color = Color::WHITE);
	void DebugDrawText3(const char* text, vec3 pos, Color color = Color::WHITE, vec2 twoDoffset = vec2::ZERO);
	void DebugDrawTriangle(vec2 p1, vec2 p2, vec2 p3, Color color = Color::WHITE);
	void DebugFillTriangle(vec2 p1, vec2 p2, vec2 p3, Color color = Color::WHITE);
	void DebugDrawTriangle3(vec3 p1, vec3 p2, vec3 p3, Color color = Color::WHITE);
	void DebugFillTriangle3(vec3 p1, vec3 p2, vec3 p3, Color color = Color::WHITE);
	void DebugDrawGraphFloat(vec2 pos, float inval, float sizex = 100, float sizey = 100);
	void AddPadding(float x);
}

#endif