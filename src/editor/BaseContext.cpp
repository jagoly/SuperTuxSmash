#include "editor/BaseContext.hpp"

#include "main/DebugGui.hpp"
#include "main/SmashApp.hpp"

#include "game/Article.hpp"
#include "game/EffectSystem.hpp"
#include "game/Emitter.hpp"
#include "game/Fighter.hpp"
#include "game/FighterAction.hpp"
#include "game/FighterState.hpp"
#include "game/HitBlob.hpp"
#include "game/ParticleSystem.hpp"
#include "game/SoundEffect.hpp"
#include "game/Stage.hpp"
#include "game/VisualEffect.hpp"
#include "game/World.hpp"

#include "render/Renderer.hpp"

#include "editor/BaseContextImpl.hpp"
#include "editor/EditorCamera.hpp"

#include <sqee/app/GuiWidgets.hpp>
#include <sqee/maths/Functions.hpp>
#include <sqee/misc/Files.hpp>
#include <sqee/objects/Armature.hpp>
#include <sqee/objects/Sound.hpp>
#include <sqee/vk/VulkanContext.hpp>

using namespace sts;
using BaseContext = EditorScene::BaseContext;

//============================================================================//

BaseContext::BaseContext(EditorScene& _editor, String _ctxKey)
    : editor(_editor), renderer(*editor.mRenderer), ctxKey(std::move(_ctxKey))
{
    sq::AudioContext& audioContext = editor.mSmashApp.get_audio_context();

    Options& options = editor.mSmashApp.get_options();
    ResourceCaches& resourceCaches = editor.mSmashApp.get_resource_caches();

    camera = std::make_unique<EditorCamera>(renderer);
    renderer.set_camera(*camera);

    world = std::make_unique<World>(options, audioContext, resourceCaches, renderer);
    world->set_rng_seed(editor.mRandomSeed);

    world->editor = std::make_unique<World::EditorData>();
    world->editor->ctxKey = ctxKey;
}

BaseContext::~BaseContext() = default;

//============================================================================//

void BaseContext::show_widget_hitblobs(const sq::Armature& armature, std::map<TinyString, HitBlobDef>& blobs)
{
    if (editor.mDoResetDockHitblobs) ImGui::SetNextWindowDockID(editor.mDockRightId);
    editor.mDoResetDockHitblobs = false;

    const ImPlus::ScopeWindow window = { "HitBlobs", 0 };
    if (window.show == false) return;

    const ImPlus::ScopeID ctxKeyScope = ctxKey.c_str();

    //--------------------------------------------------------//

    const auto funcInit = [&](HitBlobDef& blob)
    {
        // todo: should also be done for copy and rename
        for (const char* c = blob.get_key().c_str(); *c != '\0'; ++c)
            if (*c >= '0' && *c <= '9')
                { blob.index = uint8_t(*c - '0'); break; }
    };

    const auto funcEdit = [&](HitBlobDef& blob)
    {
        const ImPlus::ScopeItemWidth width = -120.f;

        ImPlus::InputValue("Index", blob.index, 1, "%u");

        ImPlus::ComboIndex("Bone", armature.get_bone_names(), blob.bone, "(None)");

        helper_edit_origin("Origin", armature, blob.bone, blob.origin);

        ImPlus::SliderValue("Radius", blob.radius, 0.05f, 2.f, "%.2f metres");

        ImPlus::InputValue("Damage", blob.damage, 1.f, "%.2f %%");
        ImPlus::InputValue("FreezeMult", blob.freezeMult, 0.1f, "%.2f ×");
        ImPlus::InputValue("FreezeDiMult", blob.freezeDiMult, 0.1f, "%.2f ×");

        ImPlus::InputValue("KnockAngle", blob.knockAngle, 1.f, "%.2f degrees");
        ImPlus::InputValue("KnockBase", blob.knockBase, 1.f, "%.2f units");
        ImPlus::InputValue("KnockScale", blob.knockScale, 1.f, "%.2f units");

        ImPlus::ComboEnum("AngleMode", blob.angleMode);
        ImPlus::ComboEnum("FacingMode", blob.facingMode);
        ImPlus::ComboEnum("ClangMode", blob.clangMode);
        ImPlus::ComboEnum("Flavour", blob.flavour);

        ImPlus::Checkbox("IgnoreDamage", &blob.ignoreDamage); ImGui::SameLine(160.f);
        ImPlus::Checkbox("IgnoreWeight", &blob.ignoreWeight);

        ImPlus::Checkbox("CanHitGround", &blob.canHitGround); ImGui::SameLine(160.f);
        ImPlus::Checkbox("CanHitAir", &blob.canHitAir);

        // todo: make these combo boxes, or at least show a warning if the key doesn't exist
        ImGui::InputText("Handler", blob.handler.data(), blob.handler.buffer_size());
        ImGui::InputText("Sound", blob.sound.data(), blob.sound.buffer_size());
    };

    helper_edit_objects(blobs, funcInit, funcEdit, nullptr);
}

//============================================================================//

void BaseContext::show_widget_effects(const sq::Armature& armature, std::map<TinyString, VisualEffectDef>& effects)
{
    if (editor.mDoResetDockEffects) ImGui::SetNextWindowDockID(editor.mDockRightId);
    editor.mDoResetDockEffects = false;

    const ImPlus::ScopeWindow window = { "Effects", 0 };
    if (window.show == false) return;

    const ImPlus::ScopeID ctxKeyScope = ctxKey.c_str();

    //--------------------------------------------------------//

    const auto funcInit = [&](VisualEffectDef& def)
    {
        if (fighter != nullptr)
        {
            // todo: try some other paths once we have common effects
            def.path = fmt::format("fighters/{}/effects/{}", fighter->def.name, def.get_key());
            def.handle = world->caches.effects.try_acquire(def.path, true);
        }
    };

    const auto funcEdit = [&](VisualEffectDef& def)
    {
        const ImPlus::ScopeItemWidth widthScope = -100.f;

        if (ImPlus::InputString("Path", def.path))
            def.handle = world->caches.effects.try_acquire(def.path, true);

        if (def.handle == nullptr) ImPlus::LabelText("Resolved", "COULD NOT LOAD RESOURCE");
        else ImPlus::LabelText("Resolved", fmt::format("assets/{}/...", def.path));

        ImPlus::ComboIndex("Bone", armature.get_bone_names(), def.bone, "(None)");

        bool changed = false;
        changed |= ImPlus::InputVector("Origin", def.origin, 0, "%.4f");
        changed |= ImPlus::InputQuaternion("Rotation", def.rotation, 0, "%.4f");
        changed |= ImPlus::InputVector("Scale", def.scale, 0, "%.4f");

        if (changed == true)
            def.localMatrix = maths::transform(def.origin, def.rotation, def.scale);

        ImPlus::Checkbox("Attached", &def.attached); ImGui::SameLine(120.f);
        ImPlus::Checkbox("Transient", &def.transient);
    };

    helper_edit_objects(effects, funcInit, funcEdit, nullptr);
}

//============================================================================//

void BaseContext::show_widget_emitters(const sq::Armature& armature, std::map<TinyString, Emitter>& emitters)
{
    if (editor.mDoResetDockEmitters) ImGui::SetNextWindowDockID(editor.mDockRightId);
    editor.mDoResetDockEmitters = false;

    const ImPlus::ScopeWindow window = { "Emitters", 0 };
    if (window.show == false) return;

    const ImPlus::ScopeID ctxKeyScope = ctxKey.c_str();

    //--------------------------------------------------------//

    const auto funcInit = [&](Emitter& emitter)
    {
        emitter.colour.emplace_back(1.f, 1.f, 1.f);
    };

    const auto funcEdit = [&](Emitter& emitter)
    {
        const ImPlus::ScopeItemWidth widthScope = -100.f;

        ImPlus::ComboIndex("Bone", armature.get_bone_names(), emitter.bone, "(None)");

        ImPlus::SliderValue("Count", emitter.count, 0u, 120u, "%u");

        helper_edit_origin("Origin", armature, emitter.bone, emitter.origin);

        ImPlus::InputVector("Velocity", emitter.velocity, 0, "%.4f");

        ImPlus::SliderValue("BaseOpacity", emitter.baseOpacity, 0.2f, 1.f, "%.3f");
        ImPlus::SliderValue("EndOpacity", emitter.endOpacity, 0.f, 5.f, "%.3f");
        ImPlus::SliderValue("EndScale", emitter.endScale, 0.f, 5.f, "%.3f");

        ImPlus::DragValueRange2("Lifetime", emitter.lifetime.min, emitter.lifetime.max, 1.f/8, 4u, 240u, "%d");
        ImPlus::DragValueRange2("BaseRadius", emitter.baseRadius.min, emitter.baseRadius.max, 1.f/320, 0.05f, 1.f, "%.3f");

        ImPlus::DragValueRange2("BallOffset", emitter.ballOffset.min, emitter.ballOffset.max, 1.f/80, 0.f, 2.f, "%.3f");
        ImPlus::DragValueRange2("BallSpeed", emitter.ballSpeed.min, emitter.ballSpeed.max, 1.f/80, 0.f, 10.f, "%.3f");

        ImPlus::DragValueRange2("DiscIncline", emitter.discIncline.min, emitter.discIncline.max, 1.f/320, -0.25f, 0.25f, "%.3f");
        ImPlus::DragValueRange2("DiscOffset", emitter.discOffset.min, emitter.discOffset.max, 1.f/80, 0.f, 2.f, "%.3f");
        ImPlus::DragValueRange2("DiscSpeed", emitter.discSpeed.min, emitter.discSpeed.max, 1.f/80, 0.f, 10.f, "%.3f");

        Vec3F* entryToDelete = nullptr;

        for (Vec3F* iter = emitter.colour.begin(); iter != emitter.colour.end(); ++iter)
        {
            const ImPlus::ScopeID idScope = iter;
            if (ImGui::Button("X")) entryToDelete = iter;
            ImGui::SameLine();
            ImPlus::InputColour("RGB (Linear)", *iter, ImGuiColorEditFlags_Float);
        }

        if (entryToDelete != nullptr)
        {
            emitter.colour.erase(entryToDelete);
            if (emitter.colour.empty())
                emitter.colour.emplace_back(1.f, 1.f, 1.f);
        }

        if (ImGui::Button("New Colour Entry") && !emitter.colour.full())
            emitter.colour.emplace_back(1.f, 1.f, 1.f);

        ImGui::InputText("Sprite", emitter.sprite.data(), emitter.sprite.capacity());
    };

    helper_edit_objects(emitters, funcInit, funcEdit, nullptr);
}

//============================================================================//

void BaseContext::show_widget_sounds(std::map<SmallString, SoundEffect>& sounds)
{
    if (editor.mDoResetDockSounds) ImGui::SetNextWindowDockID(editor.mDockRightId);
    editor.mDoResetDockSounds = false;

    const ImPlus::ScopeWindow window = { "Sounds", 0 };
    if (window.show == false) return;

    const ImPlus::ScopeID ctxKeyScope = ctxKey.c_str();

    //--------------------------------------------------------//

    const auto funcInit = [&](SoundEffect& sound)
    {
        if (fighter != nullptr)
        {
            // todo: try some other paths once we have common sounds
            sound.path = fmt::format("fighters/{}/sounds/{}", fighter->def.name, sound.get_key());
            sound.handle = world->caches.sounds.try_acquire(sound.path, true);
        }
    };

    const auto funcEdit = [&](SoundEffect& sound)
    {
        const ImPlus::ScopeItemWidth widthScope = -100.f;

        if (ImPlus::InputString("Path", sound.path))
            sound.handle = world->caches.sounds.try_acquire(sound.path, true);

        if (sound.handle == nullptr) ImPlus::LabelText("Resolved", "COULD NOT LOAD RESOURCE");
        else ImPlus::LabelText("Resolved", fmt::format("assets/{}.wav", sound.path));

        ImPlus::SliderValue("Volume", sound.volume, 0.2f, 1.f, "%.2f ×");
    };

    const auto funcBefore = [&](SoundEffect& sound)
    {
        ImGui::SetCursorPosX(ImGui::GetStyle().WindowPadding.x * 0.5f + 1.f);
        if (sound.handle == nullptr) ImGui::BeginDisabled();
        if (ImGui::Button("Play"))
            world->audio.play_sound(sound.handle.get(), sq::SoundGroup::Sfx, sound.volume, false);
        if (sound.handle == nullptr) ImGui::EndDisabled();
        ImGui::SameLine();
    };

    helper_edit_objects(sounds, funcInit, funcEdit, funcBefore);
}

//============================================================================//

void BaseContext::show_widget_scripts()
{
    if (editor.mDoResetDockScript) ImGui::SetNextWindowDockID(editor.mDockRightId);
    editor.mDoResetDockScript = false;

    const ImPlus::ScopeWindow window = { "Script", 0 };
    if (window.show == false) return;

    const ImPlus::ScopeID ctxKeyScope = ctxKey.c_str();

    //--------------------------------------------------------//

    ImGui::SetNextItemWidth(-1.f);
    ImPlus::ComboIndex("##path", std::views::transform(sourceFiles, [](auto& sf) -> const String& { return sf.path; }), sourceFileIndex);

    // imported modules can't be edited, only viewed
    const bool disableEdit = sourceFileIndex < 0 || sourceFiles[sourceFileIndex].sourcePtr == nullptr;

    if (disableEdit) ImGui::BeginDisabled();

    if (ImGui::Button("Edit"))
    {
        const SourceFile& sf = sourceFiles[sourceFileIndex];
        // todo:
        //  - add a config file that lets the user change the editor command line
        //  - canonicalise path so that qtcreator doesn't complain that the file isn't part of the project
      #if defined(SQEE_LINUX)
        // qtcreator needs the -client option to open in an existing window
        if (auto cmd = fmt::format("qtcreator -client \"{}\"", sf.path); std::system(cmd.c_str()) != 0)
            if (auto cmd = fmt::format("xdg-open \"{}\"", sf.path); std::system(cmd.c_str()) != 0)
                sq::log_warning("could not open with external editor: {}", sf.path);
      #elif defined(SQEE_WINDOWS)
        if (auto cmd = fmt::format("start \"\" \"{}\"", sf.path); std::system(cmd.c_str()) != 0)
            sq::log_warning("could not open with external editor: {}", sf.path);
      #endif
    }
    ImPlus::HoverTooltip("open with external text editor");
    ImGui::SameLine();

    // todo: replace with a "Watch" toggle to automatically reload the file
    if (ImGui::Button("Reload"))
    {
        SourceFile& sf = sourceFiles[sourceFileIndex];

        if (auto newSource = sq::try_read_text_from_file(sf.path))
        {
            *sf.sourcePtr = *newSource;
            *sf.savedSourcePtr = std::move(*newSource);
        }
        else sq::log_warning("could not read file: '{}'", sf.path);
    }
    ImPlus::HoverTooltip("reload from .wren file");
    ImGui::SameLine();

    if (ImGui::Button("Clean"))
    {
        SourceFile& sf = sourceFiles[sourceFileIndex];

        StringView src = *sf.sourcePtr;
        String result;
        do
        {
            const size_t newLine = src.find('\n');
            if (newLine == StringView::npos)
            {
                const size_t lastChar = src.find_last_not_of(' ');
                if (lastChar != StringView::npos)
                    result += src.substr(0, lastChar+1);
                src = StringView();
            }
            else if (newLine != 0)
            {
                const size_t lastChar = src.find_last_not_of(' ', newLine-1);
                if (lastChar != StringView::npos)
                    result += src.substr(0, lastChar+1);
                src.remove_prefix(newLine+1);
            }
            else src.remove_prefix(1);
            result += '\n';
        }
        while (src.find_first_not_of(" \n") != StringView::npos);

        *sf.sourcePtr = std::move(result);
    }
    ImPlus::HoverTooltip("remove trailing spaces and newlines");
    ImGui::SameLine();

    if (ImGui::Button("Enumerate"))
    {
        // todo: update this automatically when clicking the combo box if source has changed
        enumerate_source_files(std::move(sourceFiles[0].path), *sourceFiles[0].sourcePtr, *sourceFiles[0].savedSourcePtr);
    }
    ImPlus::HoverTooltip("update list of imported source files");

    if (disableEdit) ImGui::EndDisabled();

    //--------------------------------------------------------//

    const ImPlus::ScopeFont font = ImPlus::FONT_MONO;

    const ImVec2 contentRegion = ImGui::GetContentRegionAvail();
    const ImVec2 inputSize = { contentRegion.x, contentRegion.y - 160.f };

    if (sourceFileIndex >= 0)
    {
        auto& [path, sourcePtr, sourceStr, savedSourcePtr] = sourceFiles[sourceFileIndex];

        ImPlus::InputStringMultiline (
            "##source", sourcePtr ? *sourcePtr : sourceStr, inputSize,
            ImGuiInputTextFlags_NoUndoRedo | (disableEdit ? ImGuiInputTextFlags_ReadOnly : 0)
        );
    }

    if (world->editor->errorMessage.empty() == false)
        ImPlus::TextWrapped(world->editor->errorMessage);
}

//============================================================================//

void BaseContext::show_widget_timeline()
{
    if (editor.mDoResetDockTimeline) ImGui::SetNextWindowDockID(editor.mDockDownId);
    editor.mDoResetDockTimeline = false;

    const ImPlus::ScopeWindow window = { "Timeline", ImGuiWindowFlags_HorizontalScrollbar };
    if (window.show == false) return;

    const ImPlus::ScopeID ctxKeyScope = ctxKey.c_str();

    //--------------------------------------------------------//

    const ImPlus::ScopeFont font = ImPlus::FONT_MONO;

    const ImPlus::Style_ItemSpacing itemSpacing = {0.f, 0.f};
    const ImPlus::Style_SelectableTextAlign selectableAlign = {0.5f, 0.5f};

    ImDrawList* drawList = ImGui::GetWindowDrawList();

    const int numElements = timelineEnd - timelineBegin + 1;
    const int totalWidth = int(ImGui::GetContentRegionAvail().x);

    ImGui::BeginGroup();

    // currentFrame that won't change if we scrub, to prevent flicker
    const int constCurrentFrame = currentFrame;

    for (int i = 0; i < numElements; ++i)
    {
        const int frame = i + timelineBegin - 1;

        const float width = [&]() {
            if (frame == constCurrentFrame) return 32.f;
            const int index = frame > constCurrentFrame ? (i - 1) : i;
            const int unclamped = ((index+1) * (totalWidth-32) / (numElements-1)) - (index * (totalWidth-32) / (numElements-1));
            return float(std::clamp(unclamped, 8, 32));
        }();

        // give the current frame a visible label
        const String label = frame == constCurrentFrame ? fmt::format("{}###{}", i-1, frame) : fmt::format("##{}", frame);

        if (i != 0) // border between frames
        {
            ImGui::SameLine();
            const ImVec2 pos = ImGui::GetCursorScreenPos();
            drawList->AddLine({pos.x - 0.5f, pos.y + 1.f}, {pos.x - 0.5f, pos.y + 31.f}, ImGui::GetColorU32(ImGuiCol_Border));
        }

        const bool active = frame == constCurrentFrame;
        const bool open = ImPlus::IsPopupOpen(label);

        if (ImPlus::Selectable(label, active || open, 0, {width, 32.f}))
            ImPlus::OpenPopup(label);

        if (frame != constCurrentFrame)
            if (ImGui::IsItemHovered() && ImGui::IsMouseDown(ImPlus::MOUSE_RIGHT))
                scrub_to_frame(frame, true);

        ImPlus::HoverTooltip(fmt::format("{} ({})", i-1, frame));

        if (frame == constCurrentFrame)
        {
            const ImVec2 rectMin = ImGui::GetItemRectMin();
            const ImVec2 rectMax = ImGui::GetItemRectMax();

            const float blend = editor.mPreviewMode == PreviewMode::Pause ? editor.mBlendValue : 0.5f;
            const float middle = maths::mix(rectMin.x, rectMax.x, blend);

            const ImVec2 scrubberTop = { middle, rectMax.y + 2.f };
            const ImVec2 scrubberLeft = { middle - 3.f, scrubberTop.y + 8.f };
            const ImVec2 scrubberRight = { middle + 3.f, scrubberTop.y + 8.f };

            drawList->AddTriangleFilled(scrubberTop, scrubberLeft, scrubberRight, ImGui::GetColorU32(ImGuiCol_Border));
        }

        ImPlus::if_PopupContextItem(label, 0, [&]()
        {
            ImPlus::MenuItem("todo (right click to change frame)", nullptr, false, false);
        });
    }

    ImGui::EndGroup();

    const ImVec2 rectMin = ImGui::GetItemRectMin();
    const ImVec2 rectMax = ImGui::GetItemRectMax();

    drawList->AddRect(rectMin, rectMax, ImGui::GetColorU32(ImGuiCol_Border));
}

//============================================================================//

void BaseContext::show_widget_debug()
{
    // todo: make proper widget for editing and saving attributes

    if (editor.mDoResetDockDebug) ImGui::SetNextWindowDockID(editor.mDockRightId);
    editor.mDoResetDockDebug = false;

    const ImPlus::ScopeWindow window = { "Debug", 0 };
    if (window.show == false) return;

    DebugGui::show_widget_stage(world->get_stage());

    for (auto& fighter : world->get_fighters())
        DebugGui::show_widget_fighter(*fighter);

    for (auto& article : world->get_articles())
        DebugGui::show_widget_article(*article);
}

//============================================================================//

void BaseContext::helper_edit_origin(const char* label, const sq::Armature& armature, int8_t bone, Vec3F& origin)
{
    const ImPlus::ScopeID idScope = label;
    static Vec3F localOrigin = nullptr;

    if (bone < 0) ImGui::BeginDisabled();
    const bool openPopup = ImGui::Button("#");
    if (bone < 0) ImGui::EndDisabled();
    ImPlus::HoverTooltip("input in bone local space");
    ImGui::SameLine();

    if (openPopup == true)
    {
        const Mat4F boneMatrix = armature.compute_bone_matrix(armature.get_rest_sample(), bone);
        localOrigin = Vec3F(maths::inverse(boneMatrix) * Vec4F(origin, 1.f));
        ImGui::OpenPopup("popup_input_local");
        ImVec2 popupPos = ImGui::GetCursorScreenPos();
        popupPos.y -= ImGui::GetStyle().WindowPadding.y;
        ImGui::SetNextWindowPos(popupPos);
    }

    ImPlus::if_Popup("popup_input_local", 0, [&]()
    {
        if (ImGui::Button("#")) localOrigin = Vec3F();
        ImPlus::HoverTooltip("snap to bone");
        ImGui::SameLine();

        ImPlus::InputVector("##input", localOrigin, 0, "%.4f");
        const Mat4F boneMatrix = armature.compute_bone_matrix(armature.get_rest_sample(), bone);
        const Vec3F newValue = Vec3F(boneMatrix * Vec4F(localOrigin, 1.f));
        const Vec3F diff = maths::abs(newValue - origin);
        if (maths::max(diff.x, diff.y, diff.z) > 0.000001f) origin = newValue;
    });

    ImPlus::InputVector(label, origin, 0, "%.4f");
}

//============================================================================//

void BaseContext::enumerate_source_files(String path, String& ref, String& saved)
{
    sourceFiles.clear();
    sourceFiles.push_back({std::move(path), &ref, String(), &saved});
    sourceFileIndex = 0;

    const auto parse_source_file = [this](auto self, StringView src) -> void
    {
        while (src.empty() == false)
        {
            if (src.starts_with("import \"") == true)
            {
                src.remove_prefix(8);

                const size_t pathEnd = src.find('"');
                if (pathEnd == StringView::npos) break;

                String path = fmt::format("wren/{}.wren", src.substr(0, pathEnd));

                if (auto source = sq::try_read_text_from_file(path))
                {
                    sourceFiles.push_back({std::move(path), nullptr, std::move(*source), nullptr});
                    self(self, sourceFiles.back().sourceStr);
                }
                else sourceFiles.push_back({std::move(path), nullptr, "COULD NOT OPEN FILE", nullptr});

                src.remove_prefix(pathEnd+1);
            }
            else
            {
                const size_t newLine = src.find('\n');
                if (newLine == StringView::npos) break;
                src.remove_prefix(newLine+1);
            }
        }
    };

    parse_source_file(parse_source_file, ref);
}

//============================================================================//

void BaseContext::reset_objects()
{
    for (auto& fighter : world->mFighters)
        fighter->reset_everything();

    for (auto& article : world->mArticles)
        article->call_do_destroy();

    world->mArticles.clear();
}

//============================================================================//

void BaseContext::setup_state_for_action()
{
    Fighter::Attributes& attrs = fighter->attributes;
    Fighter::Variables& vars = fighter->variables;

    const SmallString& name = action->def.name;

    // todo: move to json or wren so we can properly handle extra actions

    if (name == "Brake" || name == "BrakeTurn")
    {
        //fighter.change_state(fighter.mStates.at("Dash"));
        fighter->change_state(fighter->mStates.at("Neutral"));
        fighter->play_animation(fighter->def.animations.at("DashLoop"), 0u, true);
        vars.velocity.x = attrs.dashSpeed + attrs.traction;
    }

    else if (name == "HopBack" || name == "HopForward")
    {
        fighter->change_state(fighter->mStates.at("JumpSquat"));
        fighter->play_animation(fighter->def.animations.at("JumpSquat"), 0u, true);
        fighter->mAnimPlayer.animTime = float(fighter->mAnimPlayer.animation->anim.frameCount) - 1.f;
        vars.velocity.y = std::sqrt(2.f * attrs.hopHeight * attrs.gravity) + attrs.gravity * 0.5f;
    }

    else if (name == "JumpBack" || name == "JumpForward")
    {
        fighter->change_state(fighter->mStates.at("JumpSquat"));
        fighter->play_animation(fighter->def.animations.at("JumpSquat"), 0u, true);
        fighter->mAnimPlayer.animTime = float(fighter->mAnimPlayer.animation->anim.frameCount) - 1.f;
        vars.velocity.y = std::sqrt(2.f * attrs.jumpHeight * attrs.gravity) + attrs.gravity * 0.5f;
    }

    else if (name.starts_with("Air") || name.starts_with("SpecialAir"))
    {
        fighter->change_state(fighter->mStates.at("Fall"));
        fighter->play_animation(fighter->def.animations.at("FallLoop"), 0u, true);
        attrs.gravity = 0.f;
        vars.position = Vec2F(0.f, 1.f);
    }

    else
    {
        fighter->change_state(fighter->mStates.at("Neutral"));
        fighter->play_animation(fighter->def.animations.at("NeutralLoop"), 0u, true);
    }
}

//============================================================================//

void BaseContext::scrub_to_frame(int frame, bool resetObjects)
{
    SQASSERT(fighter != nullptr && action != nullptr, "wrong context type");

    // wait for all in progress rendering to finish
    sq::VulkanContext::get().device.waitIdle();

    //mController->wren_clear_history();

    // may have already been called
    if (resetObjects == true) reset_objects();

    world->get_particle_system().clear();
    world->get_effect_system().clear();
    world->set_rng_seed(editor.mRandomSeed);

    setup_state_for_action();

    // tick once to apply starting state and animation
    world->tick();

    // don't t-pose when blending the start frame
    if (frame == -1)
    {
        for (auto& fighter : world->mFighters)
        {
            fighter->mAnimPlayer.previousSample = fighter->mAnimPlayer.currentSample;
            fighter->previous = fighter->current;
            fighter->debugPreviousPoseInfo = fighter->debugCurrentPoseInfo;
        }
    }

    // will activate the action when currentFrame >= 0
    fighter->editorStartAction = action;

    // finally, scrub to the desired frame
    editor.mSmashApp.get_audio_context().set_groups_ignored(sq::SoundGroup::Sfx, true);
    for (currentFrame = -1; currentFrame < frame; ++currentFrame)
        world->tick();
    editor.mSmashApp.get_audio_context().set_groups_ignored(sq::SoundGroup::Sfx, false);

    // this would be an assertion, but we want to avoid crashing the editor
    #ifdef SQEE_DEBUG
    if (fighter->activeAction == action && currentFrame >= 0 && world->editor->errorMessage.empty())
    {
        const int actionFrame = int(fighter->activeAction->mCurrentFrame) - 1;
        if (currentFrame != actionFrame)
            sq::log_warning("out of sync: timeline = {}, action = {}", currentFrame, actionFrame);
    }
    #endif
}

//============================================================================//

void BaseContext::advance_frame(bool previous)
{
    if (previous == false)
    {
        if (++currentFrame >= timelineEnd)
        {
            if (editor.mIncrementSeed) ++editor.mRandomSeed;
            scrub_to_frame(timelineBegin - 1, true);
        }
        else world->tick();
    }
    else
    {
        if (currentFrame < timelineBegin) scrub_to_frame(timelineEnd - 1, true);
        else scrub_to_frame(currentFrame - 1, true);
    }
}
