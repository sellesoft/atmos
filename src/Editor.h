#pragma once
#ifndef ATMOS_EDITOR_H
#define ATMOS_EDITOR_H

struct EditorSettings{
	
};

struct Editor{
	EditorSettings settings;
	
	void Init();
	void Update();
	void Reset();
	void Cleanup();
};

#endif //ATMOS_EDITOR_H