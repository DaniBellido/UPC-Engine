#include "Globals.h"
#include "ConsoleModule.h"

//-----------------------------------------------------------------------------
// Static flags to control which log categories are visible in the console.
//-----------------------------------------------------------------------------
bool ConsoleModule::showInfo = true;
bool ConsoleModule::showWarnings = true;
bool ConsoleModule::showErrors = true;

ConsoleModule::ConsoleModule() {}

//-----------------------------------------------------------------------------
// Initializes the console module.
// Logs a startup message using the global Logger system.
//-----------------------------------------------------------------------------
bool ConsoleModule::init()
{
    Logger::Log("Initializing console...");
    return true;
}

//-----------------------------------------------------------------------------
// Draws the ImGui-based console window before rendering.
// Handles log filtering, color coding, and display of messages.
//-----------------------------------------------------------------------------
void ConsoleModule::preRender()
{
    // Apply custom window colors for a distinctive visual style
    ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.35f, 0.25f, 0.55f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.45f, 0.35f, 0.65f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_TitleBgCollapsed, ImVec4(0.35f, 0.25f, 0.55f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.12f, 0.12f, 0.12f, 1.0f));

    ImGui::Begin("Console");

    //-------------------------------------------------------------------------
   // Filtering buttons
   //-------------------------------------------------------------------------
   // Each button toggles the visibility of certain message types.
   // "All" enables all filters, while the others isolate a single type.
    if (ImGui::Button("All")) { showInfo = showWarnings = showErrors = true; }
    ImGui::SameLine();
    if (ImGui::Button("Messages")) { showInfo = true;  showWarnings = showErrors = false; }
    ImGui::SameLine();
    if (ImGui::Button("Warnings")) { showWarnings = true; showInfo = showErrors = false; }
    ImGui::SameLine();
    if (ImGui::Button("Errors")) { showErrors = true; showInfo = showWarnings = false; }
    ImGui::SameLine();
    if (ImGui::Button("Clear")) { Logger::Clear(); }

    ImGui::Separator();

    //-------------------------------------------------------------------------
    // Text filter
    //-------------------------------------------------------------------------
    // Provides a text box that filters log messages by substring match.
    static ImGuiTextFilter filter;
    filter.Draw("Filter (type here)");
    ImGui::Separator();


    //-------------------------------------------------------------------------
    // Log display region
    //-------------------------------------------------------------------------
    // Displays all messages currently stored in the Logger system.
    // Applies per-type color coding and respects both type and text filters.
    const auto& logs = Logger::GetMessages();
    ImGui::BeginChild("LogRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

    for (const auto& entry : logs)
    {
        // Skip messages that don’t match active category filters
        if ((entry.type == LOG_INFO && !showInfo) ||
            (entry.type == LOG_WARNING && !showWarnings) ||
            (entry.type == LOG_ERROR && !showErrors))
            continue;

        // Skip messages that don’t match text search filter
        if (!filter.PassFilter(entry.message.c_str()))
            continue;

        // Set text color based on log type
        switch (entry.type)
        {
        case LOG_INFO:    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(180, 255, 180, 255)); break;
        case LOG_WARNING: ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 150, 255)); break;
        case LOG_ERROR:   ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 100, 100, 255)); break;
        }

        // Render log message as plain unformatted text
        ImGui::TextUnformatted(entry.message.c_str());
        ImGui::PopStyleColor();
    }

    ImGui::EndChild();
    ImGui::End();

    // Restore previous ImGui style colors
    ImGui::PopStyleColor(4);
}

//-----------------------------------------------------------------------------
// Frees resources owned by the console (currently none).
//-----------------------------------------------------------------------------
bool ConsoleModule::cleanUp()
{
    return true;
}