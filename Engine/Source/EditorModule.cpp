#include "Globals.h"
#include "EditorModule.h"
#include "D3D12Module.h"


EditorModule::EditorModule(HWND hWnd, D3D12Module* d3d12)
{
	this->hWnd = hWnd;
	this->d3d12 = d3d12;
}

bool EditorModule::init() 
{
	imGuiPass = new ImGuiPass(d3d12->getDevice(), hWnd, cpuHandle, gpuHandle);

	return true;
}

void EditorModule::preRender()
{
	imGuiPass->startFrame();
	//ImGui::ShowDemoWindow();
	//ImGui::ShowDebugLogWindow();

    // ─────────────────────────────────────────────────────────────
    //  TEST
    //  DISPLAYING A COSOLE WINDOW
    // ─────────────────────────────────────────────────────────────

    // Custom style: metallic purple header + dark gray background
    ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.35f, 0.25f, 0.55f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.45f, 0.35f, 0.65f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_TitleBgCollapsed, ImVec4(0.35f, 0.25f, 0.55f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.12f, 0.12f, 0.12f, 1.0f));

    ImGui::Begin("Console");
    {
        if (ImGui::Button("All"))
        {
            // TODO: Show all log messages
        }

        ImGui::SameLine();
        if (ImGui::Button("Messages"))
        {
            ImGui::TextUnformatted("HI");
        }

        ImGui::SameLine();
        if (ImGui::Button("Warnings"))
        {
            ImGui::TextUnformatted("WARNING");
        }

        ImGui::SameLine();
        if (ImGui::Button("Errors"))
        {
            ImGui::TextUnformatted("ERROR");
        }

        ImGui::SameLine();
        if (ImGui::Button("Clear"))
        {
            // TODO: Clear the console log buffer
        }

        ImGui::SameLine();
        if (ImGui::Button("Copy"))
        {
            // TODO: Copy all logs to clipboard
        }

        ImGui::SameLine();
        if (ImGui::Button("Save"))
        {
            // TODO: Save log output to a text file
        }

        ImGui::SameLine();
        if (ImGui::Button("Pause"))
        {
            // TODO: Pause automatic log updates
        }

        ImGui::Separator();

        // ===========================
        //  FILTER TEXT INPUT
        // ===========================
        static ImGuiTextFilter filter; // Built-in ImGui text filter
        filter.Draw("Filter (type here)");

        ImGui::Separator();

        // ===========================
        //  LOG OUTPUT REGION
        // ===========================
        ImGui::BeginChild("LogRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
        {
            // TODO: Display log messages here using ImGui::TextUnformatted()
            // Example: for (const auto& msg : logBuffer) ImGui::TextUnformatted(msg.c_str());
            // TODO: Apply color per log type (Info/Warning/Error)
            // TODO: Implement auto-scroll if enabled
        }
        ImGui::EndChild();
    }
    ImGui::End();

    ImGui::PopStyleColor(4); // Must match the number of PushStyleColor() calls

    // ─────────────────────────────────────────────────────────────
    //  END OF TEST
    // ─────────────────────────────────────────────────────────────
	
}

void EditorModule::render()
{
	imGuiPass->record(d3d12->getCommandList(), d3d12->getRenderTargetDescriptor());
}

void EditorModule::postRender()
{

}

bool EditorModule::cleanUp()
{
	delete imGuiPass;
	imGuiPass = nullptr;
	return true;
}