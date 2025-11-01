#pragma once
#include "Globals.h"
#include "Module.h"

class EditorModule : public Module
{

private:


public:

	EditorModule(HWND hWnd);
	~EditorModule() {}

	bool init() override
	{
		return true;
	}

	void update() override
	{
	}

	void preRender() override
	{
	}

	void postRender() override
	{
	}

	void render() override
	{
	}

	bool cleanUp() override
	{
		return true;
	}
};

