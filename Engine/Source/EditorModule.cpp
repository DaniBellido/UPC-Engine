#include "Globals.h"
#include "EditorModule.h"
#include "D3D12Module.h"

bool EditorModule::showInfo = true;
bool EditorModule::showWarnings = true;
bool EditorModule::showErrors = true;

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
            showInfo = true;
            showWarnings = true;
            showErrors = true;
        }

        ImGui::SameLine();
        if (ImGui::Button("Messages"))
        {
            showInfo = true;
            showWarnings = false;
            showErrors = false;
        }

        ImGui::SameLine();
        if (ImGui::Button("Warnings"))
        {
            showInfo = false;
            showWarnings = true;
            showErrors = false;
        }

        ImGui::SameLine();
        if (ImGui::Button("Errors"))
        {
            showInfo = false;
            showWarnings = false;
            showErrors = true;
        }

        ImGui::SameLine();
        if (ImGui::Button("Clear"))
        {
            Logger::Clear();
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

        const auto& logs = Logger::GetMessages();

        ImGui::BeginChild("LogRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
        {
            for (const auto& entry : logs)
            {
                // Filter by type
                if ((entry.type == LOG_INFO && !showInfo) ||
                    (entry.type == LOG_WARNING && !showWarnings) ||
                    (entry.type == LOG_ERROR && !showErrors))
                    continue;

                // Color by type
                switch (entry.type)
                {
                case LOG_INFO:    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(180, 255, 180, 255)); break;
                case LOG_WARNING: ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 150, 255)); break;
                case LOG_ERROR:   ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 100, 100, 255)); break;
                }

                std::string formatted = entry.message;
                ImGui::TextUnformatted(formatted.c_str());
                ImGui::PopStyleColor();
            }
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