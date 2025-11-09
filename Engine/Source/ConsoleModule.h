#pragma once
#include "Module.h"

//-----------------------------------------------------------------------------
// ConsoleModule is an editor tool that provides a visual logging interface.
// It displays messages collected by the Logger system, including info,
// warnings, and errors, with optional text filtering and color highlighting.
// Responsibilities:
//  - Render a scrollable ImGui console window
//  - Display log messages with color-coded categories
//  - Provide buttons for filtering (All, Messages, Warnings, Errors)
//  - Allow clearing of stored logs
//-----------------------------------------------------------------------------

class ConsoleModule : public Module
{
private:
	static bool showInfo;
	static bool showWarnings;
	static bool showErrors;

public:

    ConsoleModule();
    ~ConsoleModule() override = default;

    bool init() override;
    void preRender() override;
    void render() override {}
    void postRender() override {}
    bool cleanUp() override;

};

