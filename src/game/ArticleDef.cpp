#include "game/ArticleDef.hpp"

#include "game/Emitter.hpp"
#include "game/HitBlob.hpp"
#include "game/VisualEffect.hpp"
#include "game/World.hpp"

#include <sqee/misc/Files.hpp>
#include <sqee/misc/Json.hpp>

using namespace sts;

//============================================================================//

ArticleDef::ArticleDef(World& world, String directory)
    : EntityDef(world, directory)
{
    initialise_sounds(fmt::format("assets/{}/Sounds.json", directory));
    initialise_animations(fmt::format("assets/{}/Animations.json", directory));
}

ArticleDef::~ArticleDef()
{
    if (scriptClass) wrenReleaseHandle(world.vm, scriptClass);
}

//============================================================================//

void ArticleDef::load_json_from_file()
{
    blobs.clear();
    effects.clear();
    emitters.clear();

    const String jsonPath = fmt::format("assets/{}/Article.json", directory);

    const auto root = sq::try_parse_json_from_file(jsonPath);
    if (root.has_value() == false)
    {
        sq::log_warning("article '{}': missing json", directory);
        return;
    }

    String errors;

    TRY_FOR (const auto& item : root->at("blobs").items())
    {
        HitBlobDef& blob = blobs[item.key()];

        try { blob.from_json(item.value(), armature); }
        catch (const std::exception& ex) {
            sq::format_append(errors, "\nblob '{}': {}", item.key(), ex.what());
        }
    }
    CATCH (const std::exception& ex) { errors += '\n'; errors += ex.what(); }

    TRY_FOR (const auto& item : root->at("effects").items())
    {
        VisualEffectDef& effect = effects[item.key()];

        try { effect.from_json(item.value(), armature, world.caches.effects); }
        catch (const std::exception& ex) {
            sq::format_append(errors, "\neffect '{}': {}", item.key(), ex.what());
        }
    }
    CATCH (const std::exception& ex) { errors += '\n'; errors += ex.what(); }

    TRY_FOR (const auto& item : root->at("emitters").items())
    {
        Emitter& emitter = emitters[item.key()];

        try { emitter.from_json(item.value(), armature); }
        catch (const std::exception& ex) {
            sq::format_append(errors, "\nemitter '{}': {}", item.key(), ex.what());
        }
    }
    CATCH (const std::exception& ex) { errors += '\n'; errors += ex.what(); }

    if (errors.empty() == false)
        sq::log_warning_multiline("'{}': errors in json{}", directory, errors);
}

//============================================================================//

void ArticleDef::load_wren_from_file()
{
    const String wrenPath = fmt::format("assets/{}/Article.wren", directory);

    // set mWrenSource to either the file contents or a fallback script
    auto source = sq::try_read_text_from_file(wrenPath);
    if (source.has_value() == false)
    {
        sq::log_warning("'{}': missing script", directory);

        // articles all use the same fallback script
        const String source = sq::read_text_from_file("wren/fallback/Article.wren");
    }
    wrenSource = std::move(*source);

    // parse wrenSource and store the class handle
    interpret_module();
}

//============================================================================//

void ArticleDef::interpret_module()
{
    auto& vm = world.vm;
    auto& editor = world.editor;

    const String module = directory;

    // if the module is already loaded, unload it
    if (scriptClass != nullptr)
    {
        wrenReleaseHandle(vm, scriptClass);
        wrenUnloadModule(vm, module.c_str());
    }

    // interpret wren source string into a new module
    String errors = vm.safe_interpret(module.c_str(), wrenSource.c_str());

    // make sure that module contains a Script
    if (errors.empty() == true && wrenHasVariable(vm, module.c_str(), "Script") == false)
        errors = "module lacks a 'Script' class\n";

    // problem with the module, show error message and load fallback
    if (errors.empty() == false)
    {
        String message = fmt::format("'{}'\n{}C++ | load_wren_from_string()", directory, errors);

        if (editor == nullptr)
            sq::log_error_multiline(message);

//        else if (editor->actionKey != ActionKey{fighter.name, name})
//            sq::log_warning_multiline(message);

        else editor->errorMessage = std::move(message);

        // unload module with errors
        wrenUnloadModule(vm, module.c_str());

        // articles all use the same fallback script
        const String source = sq::read_text_from_file("wren/fallback/Article.wren");

        // fallback script is assumed not to have errors
        vm.interpret(module.c_str(), source.c_str());
    }

    // no errors, clear editor error message
//    else if (editor != nullptr && editor->actionKey == ActionKey{fighter.name, name})
//        editor->errorMessage.clear();

    // store the class for use by Article
    scriptClass = vm.get_variable(module.c_str(), "Script");

    // don't need the source anymore, so free some memory
    if (editor == nullptr) wrenSource = String();
}

//============================================================================//

bool ArticleDef::has_changes(const ArticleDef& other) const
{
    if (blobs != other.blobs) return true;
    if (effects != other.effects) return true;
    if (emitters != other.emitters) return true;
    if (wrenSource != other.wrenSource) return true;
    return false;
}

void ArticleDef::apply_changes(const ArticleDef& other)
{
    blobs.clear();
    for (const auto& [key, blob] : other.blobs)
        blobs.try_emplace(key, blob);

    effects.clear();
    for (const auto& [key, blob] : other.effects)
        effects.try_emplace(key, blob);

    emitters.clear();
    for (const auto& [key, emitter] : other.emitters)
        emitters.try_emplace(key, emitter);

    wrenSource = other.wrenSource;
}

std::unique_ptr<ArticleDef> ArticleDef::clone() const
{
    auto result = std::make_unique<ArticleDef>(world, directory);

    result->apply_changes(*this);

    return result;
}
