#pragma once

#include "editor/EditorScene.hpp"

#include <sqee/app/GuiWidgets.hpp>

namespace sts {

//============================================================================//

template <class Map, class FuncInit, class FuncEdit>
inline void EditorScene::helper_edit_objects(Map& objects, FuncInit funcInit, FuncEdit funcEdit)
{
    using Object = typename Map::mapped_type;

    if (ImGui::Button("New...")) ImGui::OpenPopup("popup_new");

    ImGui::SameLine();
    const bool collapseAll = ImGui::Button("Collapse All");

    //--------------------------------------------------------//

    ImPlus::if_Popup("popup_new", 0, [&]()
    {
        ImPlus::Text("Create New Entry:");
        TinyString newKey;
        if (ImGui::InputText("", newKey.data(), newKey.buffer_size(), ImGuiInputTextFlags_EnterReturnsTrue))
        {
            if (auto [iter, ok] = objects.try_emplace(newKey); ok)
            {
                funcInit(iter->second);
                ImGui::CloseCurrentPopup();
            }
        }
        if (ImGui::IsWindowAppearing()) ImGui::SetKeyboardFocusHere();
    });

    //--------------------------------------------------------//

    std::optional<TinyString> toDelete;
    std::optional<std::pair<TinyString, TinyString>> toRename;
    std::optional<std::pair<TinyString, TinyString>> toCopy;

    //--------------------------------------------------------//

    for (std::pair<const TinyString, Object>& item : objects)
    {
        const TinyString& key = item.first;
        Object& object = item.second;

        const ImPlus::ScopeID soundIdScope = { key.c_str() };

        if (collapseAll) ImGui::SetNextItemOpen(false);
        const bool sectionOpen = ImGui::CollapsingHeader(key);

        //--------------------------------------------------------//

        enum class Choice { None, Delete, Rename, Copy } choice {};

        ImPlus::if_PopupContextItem(nullptr, ImPlus::MOUSE_RIGHT, [&]()
        {
            if (ImGui::MenuItem("Delete...")) choice = Choice::Delete;
            if (ImGui::MenuItem("Rename...")) choice = Choice::Rename;
            if (ImGui::MenuItem("Copy...")) choice = Choice::Copy;
        });

        if (choice == Choice::Delete) ImGui::OpenPopup("popup_delete");
        if (choice == Choice::Rename) ImGui::OpenPopup("popup_rename");
        if (choice == Choice::Copy) ImGui::OpenPopup("popup_copy");

        //--------------------------------------------------------//

        ImPlus::if_Popup("popup_delete", 0, [&]()
        {
            ImPlus::Text("Delete '{}'?"_format(key));
            if (ImGui::Button("Confirm"))
            {
                toDelete = key;
                ImGui::CloseCurrentPopup();
            }
        });

        ImPlus::if_Popup("popup_rename", 0, [&]()
        {
            ImPlus::Text("Rename '{}':"_format(key));
            TinyString newKey = key;
            if (ImGui::InputText("", newKey.data(), newKey.buffer_size(), ImGuiInputTextFlags_EnterReturnsTrue))
            {
                // todo: check if newKey already exists
                toRename.emplace(key, newKey);
                ImGui::CloseCurrentPopup();
            }
            if (ImGui::IsWindowAppearing()) ImGui::SetKeyboardFocusHere();
        });

        ImPlus::if_Popup("popup_copy", 0, [&]()
        {
            ImPlus::Text("Copy '{}':"_format(key));
            TinyString newKey = key;
            if (ImGui::InputText("", newKey.data(), newKey.buffer_size(), ImGuiInputTextFlags_EnterReturnsTrue))
            {
                // todo: check if newKey already exists
                toCopy.emplace(key, newKey);
                ImGui::CloseCurrentPopup();
            }
            if (ImGui::IsWindowAppearing()) ImGui::SetKeyboardFocusHere();
        });

        //--------------------------------------------------------//

        if (sectionOpen == true) funcEdit(object);
    }

    //--------------------------------------------------------//

    if (toDelete != std::nullopt)
    {
        const auto iter = objects.find(*toDelete);
        SQASSERT(iter != objects.end(), "");
        objects.erase(iter);
    }

    if (toRename != std::nullopt)
    {
        const auto iter = objects.find(toRename->first);
        SQASSERT(iter != objects.end(), "");
        //SQASSERT(objects.find(toRename->second) == objects.end(), "");
        if (objects.find(toRename->second) == objects.end())
        {
            auto node = objects.extract(iter);
            node.key() = toRename->second;
            objects.insert(std::move(node));
        }
    }

    if (toCopy != std::nullopt)
    {
        const auto iter = objects.find(toCopy->first);
        SQASSERT(iter != objects.end(), "");
        //SQASSERT(objects.find(toCopy->second) == objects.end(), "");
        objects.try_emplace(toCopy->second, iter->second);
    }
}

//============================================================================//

} // namespace sts
