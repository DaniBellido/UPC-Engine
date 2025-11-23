#pragma once
#include "Module.h"
class Exercise1 : public Module
{
private:
	float bgColor[4] = { 1.f, 0.f, 0.f, 1.f };

public:

	void render() override;
};

