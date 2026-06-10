#include "feature_multiscripts.h"

#ifdef FEATURE_MULTIPLE_SCRIPTS

#include "imgui.h"

#include "../platform.h"

std::vector<SItemScript*> g_vecScripts;

void MultiScripts_LoadJson(nlohmann::json& jsonfile)
{
    for (auto& item : jsonfile["scripts"])
    {
        auto newItem = new SItemScript;

        newItem->index = (int32_t)g_vecScripts.size();
        newItem->filename = utf8_decode(item["filename"]);
        newItem->pixile_location = utf8_decode(item["pixile_location"]);
        newItem->name = utf8_decode(item["name"]);
        g_vecScripts.push_back(newItem);
    }
}

void MultiScripts_SaveJson(nlohmann::json& jsonfile)
{
    for (const auto& script : g_vecScripts)
    {
        jsonfile["scripts"][script->index]["name"] = utf8_encode(script->name);
        jsonfile["scripts"][script->index]["index"] = script->index;
        jsonfile["scripts"][script->index]["filename"] = utf8_encode(script->filename);
        jsonfile["scripts"][script->index]["pixile_location"] = utf8_encode(script->pixile_location);
    }
}

void MultiScripts_DrawUI()
{
    for (auto script : g_vecScripts)
    {
        ImGui::PushID(script->index);
        {
            std::string node_name;
            node_name.append(utf8_encode(script->name));
            //node_name.append();
            node_name.append("###").append(std::to_string(script->index));
            bool node_open = ImGui::TreeNodeEx(node_name.c_str(), ImGuiSelectableFlags_SpanAllColumns);
            if (node_open)
            {

            }
        }
    }
}

#endif // FEATURE_MULTIPLE_SCRIPTS
