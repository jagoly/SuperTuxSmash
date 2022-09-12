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

    const auto root = sq::try_parse_json_from_file(fmt::format("assets/{}/Article.json", directory));
    if (root.has_value() == false)
    {
        sq::log_warning("article '{}': missing json", directory);
        return;
    }

    String errors;

    const auto items_from_json = [&root, &errors](StringView key, auto& map, auto&... fromJsonArgs)
    {
        try {
            for (const auto& item : root->at(key).items())
            {
                auto& entry = map[item.key()];
                try {
                    entry.from_json(item.value(), fromJsonArgs...);
                }
                catch (const std::exception& ex) {
                    sq::format_append(errors, "\n{}[\"{}\"]: {}", key, item.key(), ex.what());
                }
            }
        } catch (const std::exception& ex) { errors += '\n'; errors += ex.what(); }
    };

    items_from_json("blobs", blobs, armature);
    items_from_json("effects", effects, armature, world.caches.effects);
    items_from_json("emitters", emitters, armature);

    if (errors.empty() == false)
        sq::log_warning_multiline("'{}': errors in json{}", directory, errors);
}

//============================================================================//

void ArticleDef::load_wren_from_file()
{
    // set mWrenSource to either the file contents or a fallback script
    auto source = sq::try_read_text_from_file(fmt::format("assets/{}/Article.wren", directory));
    if (source.has_value() == false)
    {
        sq::log_warning("'{}': missing script", directory);

        // articles all use the same fallback script
        source = sq::read_text_from_file("wren/fallback/Article.wren");
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

    const String module = fmt::format("{}/Article", directory);

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

        else if (editor->ctxKey != directory)
            sq::log_warning_multiline(message);

        else editor->errorMessage = std::move(message);

        // unload module with errors
        wrenUnloadModule(vm, module.c_str());

        // articles all use the same fallback script
        const String source = sq::read_text_from_file("wren/fallback/Article.wren");

        // fallback script is assumed not to have errors
        vm.interpret(module.c_str(), source.c_str());
    }

    // no errors, clear editor error message
    else if (editor != nullptr && editor->ctxKey == directory)
        editor->errorMessage.clear();

    // store the class for use by Article
    scriptClass = vm.get_variable(module.c_str(), "Script");

    // don't need the source anymore, so free some memory
    if (editor == nullptr) wrenSource = String();
}
