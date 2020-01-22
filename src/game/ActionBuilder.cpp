#include "game/ActionBuilder.hpp"

#include "game/Actions.hpp"
#include "game/ActionFuncs.hpp"
#include "game/Fighter.hpp"

#include <sqee/helpers.hpp>
#include <sqee/debug/Logging.hpp>
#include <sqee/misc/Files.hpp>
#include <sqee/misc/Json.hpp>
#include <sqee/misc/Parsing.hpp>

#include <fstream> // for loading json

using json = nlohmann::json;
using sq::literals::operator""_fmt_;
using namespace sts;

//============================================================================//

namespace { // anonymous

//----------------------------------------------------------------------------//

template <class Type> Type impl_convert_param(StringView token);

template<> int impl_convert_param(StringView token)
{
    try { return sq::sv_to_i(token); }
    catch (std::invalid_argument&) { throw "Could not convert '%s' to int"_fmt_(token); }
}

template<> uint impl_convert_param(StringView token)
{
    try { return sq::sv_to_u(token); }
    catch (std::invalid_argument&) { throw "Could not convert '%s' to uint"_fmt_(token); }
}

template<> float impl_convert_param(StringView token)
{
    try { return sq::sv_to_f(token); }
    catch (std::invalid_argument&) { throw "Could not convert '%s' to float"_fmt_(token); }
}

template<> PoolKey impl_convert_param(StringView token)
{
    if (token.size() > 15u) throw "string '%s' too big for a PoolKey"_fmt_(token);
    return PoolKey(token);
}

//----------------------------------------------------------------------------//

template <class... Types> struct TypePack
{
    constexpr static size_t Size = sizeof...(Types);
    using Tuple = std::tuple<Types...>;
};

template <class Types, size_t Index>
using TypePackElement = std::tuple_element_t<Index, typename Types::Tuple>;

//----------------------------------------------------------------------------//

template <class Types, size_t Index> inline
auto impl_extract_param(const Vector<StringView>& tokens)
{
    using Type = TypePackElement<Types, Index>;
    const StringView& token = tokens[Index + 1];
    return impl_convert_param<Type>(token);
}

template <class Func, class Types, size_t... Index> inline
Action::Command impl_bind_params(Func func, const Vector<StringView>& tokens, Types, std::index_sequence<Index...>)
{
    // we have already done validation, so we don't need to handle exceptions here
    return std::bind(func, std::placeholders::_1, impl_extract_param<Types, Index>(tokens)...);
}

template <class Func, class Types, size_t... Index> inline
String impl_validate_command(Func func, Action& action, const Vector<StringView>& tokens, Types, std::index_sequence<Index...>)
{
    if (tokens.size() != Types::Size + 1u) return "wrong number of paramaters for '%s'"_fmt_(tokens[0]);

    // will throw a String if a paramater cannot be converted
    try { return func(action, impl_extract_param<Types, Index>(tokens)...); }
    catch (const String& error) { return error; }
}

template <class Func, class Validate, class... Types> inline
Action::Command impl_build_command(Func func, Validate validate, TypePack<Types...> types, Action& action,
                                   const Vector<StringView>& tokens, Vector<String>& errors, uint line)
{
    constexpr const auto indices = std::index_sequence_for<Types...>();
    const String error = impl_validate_command(validate, action, tokens, types, indices);
    if (!error.empty()) { errors.push_back("%02d: %s"_fmt_(line, error)); return nullptr; }
    return impl_bind_params(func, tokens, types, indices);
}

//----------------------------------------------------------------------------//

} // anonymous namespace

//============================================================================//

Action::Command ActionBuilder::build_command(Action& action, StringView source, Vector<String>& errors, uint line)
{
    const auto tokens = sq::tokenise_string_view(source, ' ');

    // can happen if source is only space characters, not an error
    if (tokens.empty()) return nullptr;

    //--------------------------------------------------------//

    #define RETURN_BOUND_FUNCTION(Name, ...) \
    return impl_build_command(&ActionFuncs::Name, &ActionFuncsValidate::Name, TypePack<__VA_ARGS__ >(), action, tokens, errors, line)

    if (tokens[0] == "enable_blob")    RETURN_BOUND_FUNCTION(enable_blob, PoolKey);
    if (tokens[0] == "disable_blob")   RETURN_BOUND_FUNCTION(disable_blob, PoolKey);
    if (tokens[0] == "add_velocity")   RETURN_BOUND_FUNCTION(add_velocity, float, float);
    if (tokens[0] == "set_position")   RETURN_BOUND_FUNCTION(set_position, float, float);
    if (tokens[0] == "finish_action")  RETURN_BOUND_FUNCTION(finish_action, );
    if (tokens[0] == "emit_particles") RETURN_BOUND_FUNCTION(emit_particles, PoolKey, uint);

    #undef RETURN_BOUND_FUNCTION

    //--------------------------------------------------------//

    errors.push_back("%02d: unknown function '%s'"_fmt_(line, tokens[0]));

    return nullptr;
}

//============================================================================//

Vector<Action::Command> ActionBuilder::build_procedure(Action& action, StringView source, Vector<String>& errors)
{
    const auto sourceLines = sq::tokenise_string_view_lines(source);

    const auto oldErrorCount = errors.size();

    Vector<Action::Command> result;
    result.reserve(sourceLines.size());

    for (auto& [line, source] : sourceLines)
    {
        auto boundFunc = build_command(action, source, errors, line);
        if (boundFunc) result.push_back(std::move(boundFunc));
    }

    // remove this line to still run any commands that did build
    if (errors.size() != oldErrorCount) result.clear();

    return result;
}

//============================================================================//

void ActionBuilder::load_from_json(Action& action)
{
    action.blobs.clear(); // destroy existing blobs
    action.emitters.clear(); // destroy existing emitters
    action.procedures.clear(); // clear existing procedures

    // use a dummy action for actions I haven't written yet
    if (sq::check_file_exists(action.path) == false)
    {
        sq::log_warning("missing action '%s'", action.path);

        Action::Procedure& procedure = action.procedures["FinishAction"];

        procedure.meta.source = "finish_action";
        procedure.meta.frames = { 12u };

        Vector<String> errors; const uint line = 0u;
        procedure.commands = { build_command(action, procedure.meta.source, errors, line) };

        action.rebuild_timeline();

        return;
    }

    //--------------------------------------------------------//

    const JsonValue root = sq::parse_json_from_file(action.path);

    Vector<String> errors;

    //--------------------------------------------------------//

    try {
    for (auto iter : root.at("blobs").items())
    {
        HitBlob& blob = action.blobs[iter.key()];

        blob.fighter = &action.fighter;
        blob.action = &action;

        try { blob.from_json(iter.value()); }
        catch (const std::exception& e) {
            errors.push_back("blob '%s': %s"_fmt_(iter.key(), e.what()));
        }
    }
    } catch (const std::exception& e) { errors.emplace_back(e.what()); }

    //--------------------------------------------------------//

    try {
    for (auto iter : root.at("emitters").items())
    {
        ParticleEmitter& emitter = action.emitters[iter.key()];

        emitter.fighter = &action.fighter;
        emitter.action = &action;

        try { emitter.from_json(iter.value()); }
        catch (const std::exception& e) {
            errors.push_back("emitter '%s': %s"_fmt_(iter.key(), e.what()));
        }
    }
    } catch (const std::exception& e) { errors.emplace_back(e.what()); }

    //--------------------------------------------------------//

    try {
    for (auto iter : root.at("procedures").items())
    {
        Action::Procedure& procedure = action.procedures[iter.key()];

        try { iter.value().at("source").get_to(procedure.meta.source); }
        catch (const std::exception& e) {
            errors.push_back("procedure '%s': %s"_fmt_(iter.key(), e.what()));
        }

        try { iter.value().at("frames").get_to(procedure.meta.frames); }
        catch (const std::exception& e) {
            errors.push_back("procedure '%s': %s"_fmt_(iter.key(), e.what()));
        }

        procedure.commands = build_procedure(action, procedure.meta.source, errors);
    }
    } catch (const std::exception& e) { errors.emplace_back(e.what()); }

    //--------------------------------------------------------//

    if (errors.empty() == false)
        sq::log_warning_block("errors in action '%s'"_fmt_(action.path), errors);

    action.rebuild_timeline();
}

//============================================================================//

JsonValue ActionBuilder::serialise_as_json(const Action& action)
{
    JsonValue result;

    //--------------------------------------------------------//

    auto& resultBlobs = result["blobs"] = JsonValue::object();

    for (const auto& [key, blob] : action.blobs)
        blob.to_json(resultBlobs[key]);

    auto& resultEmitters = result["emitters"] = JsonValue::object();

    for (const auto& [key, emitter] : action.emitters)
        emitter.to_json(resultEmitters[key]);

    //--------------------------------------------------------//

    auto& resultProcedures = result["procedures"] = JsonValue::object();

    for (const auto& [key, procedure] : action.procedures)
    {
        JsonValue& jsonProcedure = resultProcedures[key];
        jsonProcedure["source"] = procedure.meta.source;
        jsonProcedure["frames"] = procedure.meta.frames;
    }

    //--------------------------------------------------------//

    return result;
}
