#include "editor/ArticleContext.hpp"

#include "game/Article.hpp"
#include "game/Emitter.hpp"
#include "game/Fighter.hpp"
#include "game/FighterAction.hpp"
#include "game/HitBlob.hpp"
#include "game/SoundEffect.hpp"
#include "game/VisualEffect.hpp"
#include "game/World.hpp"

#include <sqee/app/GuiWidgets.hpp>
#include <sqee/misc/Files.hpp>
#include <sqee/misc/Json.hpp>

using namespace sts;
using ArticleContext = EditorScene::ArticleContext;

//============================================================================//

ArticleContext::ArticleContext(EditorScene& _editor, String _ctxKey)
    : BaseContext(_editor, std::move(_ctxKey))
{
    const auto document = JsonDocument::parse_file(fmt::format("assets/{}/Editor.json", ctxKey));
    const auto json = document.root().as<JsonObject>();

    const auto fighterKey = json["defaultFighter"].as<StringView>();
    const auto actionKey = json["defaultAction"].as<StringView>();

    world->create_stage("TestZone");

    fighter = &world->create_fighter(fighterKey);
    fighter->controller = editor.mController.get();

    action = &fighter->mActions.at(actionKey);

    articleDef = const_cast<ArticleDef*>(&world->load_article_def(ctxKey));

    savedData = std::make_unique<UndoEntry>(*articleDef);
    undoStack.push_back(std::make_unique<UndoEntry>(*articleDef));

    timelineBegin = json["timelineBegin"].as_auto();
    timelineEnd = json["timelineEnd"].as_auto();

    enumerate_source_files(fmt::format("assets/{}/Article.wren", ctxKey), articleDef->wrenSource, savedData->wrenSource);

    scrub_to_frame(timelineBegin - 1, false);
}

ArticleContext::~ArticleContext() = default;

//============================================================================//

void ArticleContext::apply_working_changes()
{
    if (undoStack[undoIndex]->has_changes(*articleDef) == true)
    {
        reset_objects();

        articleDef->interpret_module();

        scrub_to_frame(currentFrame, false);

        undoStack.erase(undoStack.begin() + (++undoIndex), undoStack.end());
        undoStack.push_back(std::make_unique<UndoEntry>(*articleDef));

        modified = savedData->has_changes(*articleDef);
    }
}

//============================================================================//

void ArticleContext::do_undo_redo(bool redo)
{
    const size_t oldIndex = undoIndex;

    if (!redo && undoIndex > 0u) --undoIndex;
    if (redo && undoIndex < undoStack.size() - 1u) ++undoIndex;

    if (undoIndex != oldIndex)
    {
        reset_objects();

        undoStack[undoIndex]->revert_changes(*articleDef);

        articleDef->interpret_module();

        scrub_to_frame(currentFrame, false);

        modified = savedData->has_changes(*articleDef);
    }
}

//============================================================================//

void ArticleContext::save_changes()
{
    if (savedData->sounds != articleDef->sounds)
    {
        auto document = JsonMutDocument();
        auto json = document.assign(JsonMutObject(document));

        for (const auto& [key, sound] : articleDef->sounds)
            sound.to_json(json.append(key, JsonMutObject(document)));

        sq::write_text_to_file(fmt::format("assets/{}/Sounds.json", ctxKey), json.dump(true), true);
    }

    if (savedData->blobs != articleDef->blobs || savedData->effects != articleDef->effects || savedData->emitters != articleDef->emitters)
    {
        auto document = JsonMutDocument();
        auto json = document.assign(JsonMutObject(document));

        auto jBlobs = json.append("blobs", JsonMutObject(document));
        auto jEffects = json.append("effects", JsonMutObject(document));
        auto jEmitters = json.append("emitters", JsonMutObject(document));

        for (const auto& [key, blob] : articleDef->blobs)
            blob.to_json(jBlobs.append(key, JsonMutObject(document)), articleDef->armature);

        for (const auto& [key, effect] : articleDef->effects)
            effect.to_json(jEffects.append(key, JsonMutObject(document)), articleDef->armature);

        for (const auto& [key, emitter] : articleDef->emitters)
            emitter.to_json(jEmitters.append(key, JsonMutObject(document)), articleDef->armature);

        sq::write_text_to_file(fmt::format("assets/{}/Article.json", ctxKey), json.dump(true), true);
        sq::write_text_to_file(fmt::format("assets/{}/Article.wren", ctxKey), articleDef->wrenSource, true);
    }

    savedData = std::make_unique<UndoEntry>(*articleDef);
    modified = false;
}

//============================================================================//

void ArticleContext::show_menu_items()
{
    if (ImGui::MenuItem("Reload Animations"))
    {
        // todo: make sure there are no dangling pointers around
        articleDef->animations.clear();
        articleDef->initialise_animations(fmt::format("assets/{}/Animations.json", ctxKey));
        deferScrubToFrame = currentFrame;
    }
}

//============================================================================//

void ArticleContext::show_widgets()
{
    show_widget_sounds(articleDef->sounds);
    show_widget_hitblobs(articleDef->armature, articleDef->blobs);
    show_widget_effects(articleDef->armature, articleDef->effects);
    show_widget_emitters(articleDef->armature, articleDef->emitters);
    show_widget_scripts();
    show_widget_timeline();
    show_widget_debug();
}

//============================================================================//

ArticleContext::UndoEntry::UndoEntry(const ArticleDef& def)
    : sounds(def.sounds)
    , blobs(def.blobs)
    , effects(def.effects)
    , emitters(def.emitters)
    , wrenSource(def.wrenSource) {}

bool ArticleContext::UndoEntry::has_changes(const ArticleDef& def) const
{
    return def.sounds != sounds ||
           def.blobs != blobs ||
           def.effects != effects ||
           def.emitters != emitters ||
           def.wrenSource != wrenSource;
}

void ArticleContext::UndoEntry::revert_changes(ArticleDef& def) const
{
    def.sounds = sounds;
    def.blobs = blobs;
    def.effects = effects;
    def.emitters = emitters;
    def.wrenSource = wrenSource;
}
