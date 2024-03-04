#pragma once

#include "editor/BaseContext.hpp"

#include <sqee/app/GuiWidgets.hpp>

namespace sts {

//============================================================================//

template <class Key, class Object, class FuncInit, class FuncEdit, class FuncBefore>
inline void EditorScene::BaseContext::helper_edit_objects (
    std::map<Key, Object>& objects, FuncInit funcInit, FuncEdit funcEdit, FuncBefore funcBefore
)
{
    if (ImGui::Button("New...")) ImGui::OpenPopup("popup_new");

    ImGui::SameLine();
    const bool collapseAll = ImGui::Button("Collapse All");

    ImPlus::if_Popup("popup_new", 0, [&]()
    {
        ImPlus::Text("Create New Entry:");
        Key newKey;
        if (ImGui::InputText("##input", newKey.data(), newKey.buffer_size(), ImGuiInputTextFlags_EnterReturnsTrue))
        {
            if (auto [iter, ok] = objects.try_emplace(newKey); ok)
            {
                funcInit(iter->second);
                ImGui::CloseCurrentPopup();
            }
        }
        if (ImGui::IsWindowAppearing()) ImGui::SetKeyboardFocusHere();
    });

    using Iterator = typename std::map<Key, Object>::iterator;

    std::optional<Iterator> toDelete;
    std::optional<std::pair<Iterator, Key>> toRename;
    std::optional<std::pair<Iterator, Key>> toCopy;

    //--------------------------------------------------------//

    for (Iterator iter = objects.begin(); iter != objects.end(); ++iter)
    {
        const Key& key = iter->first;
        Object& object = iter->second;

        IMPLUS_WITH(Scope_ID) = ImStrv(key);

        if constexpr (std::is_same_v<FuncBefore, std::nullptr_t> == false)
            funcBefore(object);

        if (collapseAll) ImGui::SetNextItemOpen(false);
        const bool sectionOpen = ImGui::CollapsingHeader(key);

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

        ImPlus::if_Popup("popup_delete", 0, [&]()
        {
            ImPlus::Text(fmt::format("Delete '{}'?", key));
            if (ImGui::Button("Confirm"))
            {
                toDelete = iter;
                ImGui::CloseCurrentPopup();
            }
        });

        ImPlus::if_Popup("popup_rename", 0, [&]()
        {
            ImPlus::Text(fmt::format("Rename '{}':", key));
            Key newKey = key;
            if (ImGui::InputText("##input", newKey.data(), newKey.buffer_size(), ImGuiInputTextFlags_EnterReturnsTrue))
            {
                toRename.emplace(iter, newKey);
                ImGui::CloseCurrentPopup();
            }
            if (ImGui::IsWindowAppearing()) ImGui::SetKeyboardFocusHere();
        });

        ImPlus::if_Popup("popup_copy", 0, [&]()
        {
            ImPlus::Text(fmt::format("Copy '{}':", key));
            Key newKey = key;
            if (ImGui::InputText("##input", newKey.data(), newKey.buffer_size(), ImGuiInputTextFlags_EnterReturnsTrue))
            {
                toCopy.emplace(iter, newKey);
                ImGui::CloseCurrentPopup();
            }
            if (ImGui::IsWindowAppearing()) ImGui::SetKeyboardFocusHere();
        });

        if (sectionOpen == true) funcEdit(object);
    }

    //--------------------------------------------------------//

    if (toDelete.has_value() == true)
        objects.erase(*toDelete);

    if (toRename.has_value() == true)
    {
        if (objects.find(toRename->second) == objects.end())
        {
            auto node = objects.extract(toRename->first);
            node.key() = toRename->second;
            objects.insert(std::move(node));
        }
    }

    if (toCopy.has_value() == true)
        objects.try_emplace(toCopy->second, toCopy->first->second);
}

//============================================================================//

} // namespace sts
