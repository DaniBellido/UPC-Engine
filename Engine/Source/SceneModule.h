#pragma once
#include "Module.h"
class SceneModule : public Module
{
public:
	SceneModule();
	~SceneModule();

	bool init() override;
	void update() override;
	void preRender() override;
	void render() override;
	void postRender() override;
	bool cleanUp() override;
};

