#include "Globals.h"
#include "EditorModule.h"
#include "D3D12Module.h"
#include "Application.h"


enum class ExerciseSelection
{
	None = 0,
	Exercise1,
	Exercise2,
	Exercise3,
	Exercise4,
	Exercise5,
	// Add more exercises
};

ExerciseSelection currentExercise = ExerciseSelection::None;

EditorModule::EditorModule(HWND hWnd, D3D12Module* d3d12)
{
	this->hWnd = hWnd;
	this->d3d12 = d3d12;
	
}

bool EditorModule::init() 
{
	Logger::Log("Initializing EditorModule...");
	Timer t;
	t.Start();

	imGuiPass = new ImGuiPass(d3d12->getDevice(), hWnd, cpuHandle, gpuHandle);

	console = new ConsoleModule();
	console->init();

	viewport = new ViewportModule(hWnd, d3d12, imGuiPass);
	viewport->init();

	exercise = new ExerciseModule(d3d12);
	exercise->init();


	t.Stop();
	Logger::Log("EditorModule initialized in: " + std::to_string(t.ReadMs()) + " ms.");

	return true;
}

void EditorModule::preRender()
{
	imGuiPass->startFrame();
    //createDockSpace();
	drawToolbar();

	if (console->isVisible())
		console->preRender();

	if (viewport->isVisible())
		viewport->preRender();

	if (showExercisesWindow)
		drawExerciseMenu();

	if (showPerformancePanel)
		drawPerformancePanel();
	
}

void EditorModule::render()
{
	// Get render descriptor
	auto rtvHandle = d3d12->getRenderTargetDescriptor();

	// RGBA between 0 and 1
	const float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };

	// Clear the current RTV
	d3d12->getCommandList()->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

	// Execute selected exercise
	switch (currentExercise)
	{
	case ExerciseSelection::Exercise1:
		exercise->exercise1();
		break;

	case ExerciseSelection::Exercise2:
		exercise->exercise2();   
		break;

	case ExerciseSelection::Exercise3:
		exercise->exercise3();
		break;

	case ExerciseSelection::Exercise4:
		exercise->exercise4();
		break;

	case ExerciseSelection::Exercise5:
		exercise->exercise5();
		break;

	default:
		break;
	}

	// This must be the last call
	imGuiPass->record(d3d12->getCommandList(), d3d12->getRenderTargetDescriptor());

}

void EditorModule::postRender()
{

}

bool EditorModule::cleanUp()
{
	console->cleanUp();
	viewport->cleanUp();
	exercise->cleanUp();
	delete console;
	delete viewport;
	delete exercise;
	delete imGuiPass;
	return true;
}

void EditorModule::createDockSpace() 
{
	// Set initial dockspace flags to none (you can modify these later for behavior like no split, no resize, etc.)
	static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

	// Define the base window flags. 
	// - ImGuiWindowFlags_MenuBar: enables a menu bar region at the top of the window.
	// - ImGuiWindowFlags_NoDocking: prevents this window itself from being docked into another.
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;

	// Get information about the main application viewport (the main display area).
	const ImGuiViewport* viewport = ImGui::GetMainViewport();

	// Make the next window cover the entire main viewport area.
	ImGui::SetNextWindowPos(viewport->Pos);
	ImGui::SetNextWindowSize(viewport->Size);
	ImGui::SetNextWindowViewport(viewport->ID);

	// Add more flags so the main dockspace window behaves like a background container:
	// - NoTitleBar: removes title bar.
	// - NoCollapse: prevents collapsing the window.
	// - NoResize / NoMove: disables resizing or moving.
	// - NoBringToFrontOnFocus: keeps this window always in the background.
	// - NoNavFocus: prevents it from receiving keyboard/gamepad focus.
	// - NoBackground: ?
	window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
		ImGuiWindowFlags_NoBackground;

	// Push style modifications so the window looks flat and fullscreen:
	// - WindowRounding = 0: removes rounded corners.
	// - WindowBorderSize = 0: removes border lines.
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	//ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));

	// Begin a new invisible fullscreen window that acts as the dockspace host.
	ImGui::Begin("DockSpace Window", nullptr, window_flags);

	// Restore the previous style settings.
	ImGui::PopStyleVar(2);

	// Create a unique ID for the dockspace (required for persistent layouts).
	ImGuiID dockspace_id = ImGui::GetID("EditorDockSpace");

	// Create the DockSpace itself — this defines the area where other ImGui windows can dock.
	// The second parameter (ImVec2(0,0)) means it fills all available space.
	// The third parameter defines behavior flags (from dockspace_flags).
	ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);

	// Here you could render dockable windows like a console, inspector, scene view, etc.


	// End the dockspace window definition.
	ImGui::End();
}

static bool showAboutWindow = false;
static bool showCameraControlsWindow = true;

void EditorModule::drawToolbar()
{
	if (ImGui::BeginMainMenuBar())
	{
		// --- File ---
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("New")) { /* TODO */ }
			if (ImGui::MenuItem("Open...")) { /* TODO */ }
			if (ImGui::MenuItem("Save")) { /* TODO */ }
			ImGui::Separator();
			if (ImGui::MenuItem("Exit")) { /* TODO: exit app */ }
			ImGui::EndMenu();
		}

		// --- View ---
		if (ImGui::BeginMenu("View"))
		{
			bool showConsole = console->isVisible();
			bool showViewport = viewport->isVisible();
			if (ImGui::MenuItem("Show Console", nullptr, showConsole)) { console->setVisible(!showConsole); }
			if (ImGui::MenuItem("Show Viewport", nullptr, showViewport)) { viewport->setVisible(!showViewport); }
			if (ImGui::MenuItem("Show Exercise List", nullptr, showExercisesWindow)) { showExercisesWindow = !showExercisesWindow; }
			if (ImGui::MenuItem("Show Performance Panel", nullptr, showPerformancePanel)) { showPerformancePanel = !showPerformancePanel; }
			ImGui::EndMenu();
		}

		// --- Help ---
		if (ImGui::BeginMenu("Help"))
		{
			if (ImGui::MenuItem("About")) showAboutWindow = true;
			if (ImGui::MenuItem("Camera Controls")) showCameraControlsWindow = true;
			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();
	}

	// --- About window ---
	if (showAboutWindow)
	{
		ImGui::Begin("About", &showAboutWindow, ImGuiWindowFlags_AlwaysAutoResize);
		ImGui::Text("Project: DirectX 12 Editor");
		ImGui::Separator();
		ImGui::Text("Developed by Dani");
		ImGui::Text("Master's Program: Advanced Programming for AAA Videogames");
		ImGui::Text("UPC - 2025");
		ImGui::Separator();
		ImGui::TextWrapped(
			"This project uses Dear ImGui and DirectX 12.\n"
			"It aims to build a modular game editor framework."
		);
		ImGui::Spacing();
		if (ImGui::Button("OK")) showAboutWindow = false;
		ImGui::End();
	}

	// --- Camera Controls window ---
	if (showCameraControlsWindow)
	{
		ImGui::Begin("Camera Controls", &showCameraControlsWindow, ImGuiWindowFlags_AlwaysAutoResize);

		ImGui::Text("Mouse Controls:");
		ImGui::BulletText("Right Mouse Button + Move: Rotate camera");
		ImGui::BulletText("Mouse Wheel: Zoom in/out");
		ImGui::BulletText("Alt + Left Mouse Button + Move: Orbit around origin");

		ImGui::Separator();

		ImGui::Text("Keyboard Controls:");
		ImGui::BulletText("W / S: Move forward/backward");
		ImGui::BulletText("A / D: Move left/right");
		ImGui::BulletText("Q / E: Move down/up");
		ImGui::BulletText("Left Shift: Hold to double movement speed");
		ImGui::BulletText("F: Reset camera focus to origin");

		ImGui::End();
	}
}

void EditorModule::drawExerciseMenu()
{
	if (!showExercisesWindow)
		return;

	ImGui::Begin("Exercise List", &showExercisesWindow);

	ImGui::Text("Select an exercise:");
	ImGui::Separator();

	if (ImGui::Selectable("Exercise 1: Background Color", currentExercise == ExerciseSelection::Exercise1))
	{
		Logger::Log("Selectable 1 clicked!");
		currentExercise = ExerciseSelection::Exercise1;
	}

	if (ImGui::Selectable("Exercise 2: 2D Triangle", currentExercise == ExerciseSelection::Exercise2))
	{
		Logger::Log("Selectable 2 clicked!");
		currentExercise = ExerciseSelection::Exercise2;
	}

	if (ImGui::Selectable("Exercise 3: 3D Triangle", currentExercise == ExerciseSelection::Exercise3))
	{
		Logger::Log("Selectable 3 clicked!");
		currentExercise = ExerciseSelection::Exercise3;
	}

	if (ImGui::Selectable("Exercise 4: Textured Cube", currentExercise == ExerciseSelection::Exercise4))
	{
		Logger::Log("Selectable 4 clicked!");
		currentExercise = ExerciseSelection::Exercise4;
	}

	if (ImGui::Selectable("Exercise 5: Geometry Loading", currentExercise == ExerciseSelection::Exercise5))
	{
		Logger::Log("Selectable 5 clicked!");
		currentExercise = ExerciseSelection::Exercise5;
	}

	// Add more:
	// if (ImGui::Selectable("Exercise 3", currentExercise == ExerciseSelection::Exercise3)) { ... }

	ImGui::End();
}

void EditorModule::drawPerformancePanel()
{
	if (!showPerformancePanel)
		return;

	ImGui::Begin("Performance Panel", &showPerformancePanel);

	// --- FPS ---
	float fps = app->getFPS();
	float ms = (fps > 0.0f) ? 1000.0f / fps : 0.0f;

	ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "FPS: %.1f", fps);
	ImGui::SameLine();
	ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), " (%.1f ms)", ms);

	float fpsNorm = ImClamp(fps / 144.0f, 0.0f, 1.0f);
	ImVec4 fpsColor = ImLerp(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), ImVec4(0.3f, 1.0f, 0.3f, 1.0f), fpsNorm);
	ImGui::PushStyleColor(ImGuiCol_PlotHistogram, fpsColor);
	ImGui::ProgressBar(fpsNorm, ImVec2(-1, 12), "GPU Load");
	ImGui::PopStyleColor();

	// FPS history
	static float history[90] = {};
	static int historyOffset = 0;
	history[historyOffset] = fps;
	historyOffset = (historyOffset + 1) % IM_ARRAYSIZE(history);
	ImGui::PlotLines("##fps", history, IM_ARRAYSIZE(history), historyOffset, nullptr, 30.0f, 144.0f, ImVec2(0, 40));

	// --- Time Breakdown ---
	if (ImGui::CollapsingHeader("Time Breakdown", ImGuiTreeNodeFlags_DefaultOpen))
	{
		// Columns
		ImGui::Columns(3, "PerfColumns", false);
		ImGui::SetColumnWidth(0, 100); // Process
		ImGui::SetColumnWidth(1, 70);  // Time in ms
		ImGui::SetColumnWidth(2, 220); // Load

		// Headers
		ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "Process"); ImGui::NextColumn();
		ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "ms"); ImGui::NextColumn();
		ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "Load"); ImGui::NextColumn();
		ImGui::Separator();

		// Data
		const char* names[4] = { "Update", "PreRender", "Render", "PostRender" };
		float values[4] = { app->getUpdateMs(), app->getPreRenderMs(), app->getRenderMs(), app->getPostRenderMs() };
		ImVec4 colors[4] = {
			ImVec4(1.0f,0.3f,0.3f,1.0f),
			ImVec4(1.0f,1.0f,0.3f,1.0f),
			ImVec4(0.3f,0.3f,1.0f,1.0f),
			ImVec4(0.3f,1.0f,0.3f,1.0f)
		};

		float maxValue = 33.0f; // max timeto normalize bar
		static float displayed[4] = { 0 }; // smooth

		for (int i = 0; i < 4; i++)
		{
			// Process name
			ImGui::Text("%s", names[i]); ImGui::NextColumn();

			// Time in ms
			ImGui::Text("%.3f", values[i]); ImGui::NextColumn();

			// Smoothed bar
			float norm = ImClamp(values[i] / maxValue, 0.0f, 1.0f);
			displayed[i] = displayed[i] * 0.9f + norm * 0.1f;

			ImGui::PushStyleColor(ImGuiCol_PlotHistogram, colors[i]);
			ImGui::ProgressBar(displayed[i], ImVec2(-1, 12));
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("%.3f ms (%.0f%% load)", values[i], displayed[i] * 100.0f);
			ImGui::PopStyleColor();
			ImGui::NextColumn();
		}

		ImGui::Columns(1);
	}

	ImGui::End();
}


