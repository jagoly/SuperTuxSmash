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
    if (tokens.size() != Types::Size + 1u) return "wrong number of paramaters for command";

    // will throw a String if a paramater cannot be converted
    try { return func(action, impl_extract_param<Types, Index>(tokens)...); }
    catch (const String& error) { return error; }
}

//----------------------------------------------------------------------------//

} // anonymous namespace

//============================================================================//

template <class... Args>
void ActionBuilder::impl_log_error(const char* fmt, const Args&... args)
{
    mErrorLog.emplace_back(tfm::format(fmt, args...));
}

//============================================================================//

Action::Command ActionBuilder::build_command(Action& action, StringView source)
{
    const auto tokens = sq::tokenise_string_view(source, ' ');

    if (tokens.empty() == true)
    {
        mErrorLog.emplace_back("source is empty");
        return nullptr;
    }

    //--------------------------------------------------------//

    #define RETURN_BOUND_FUNCTION(Name, ...) \
    do { \
        constexpr const auto types = TypePack<__VA_ARGS__ >(); \
        constexpr const auto indices = std::index_sequence_for<__VA_ARGS__>(); \
        String error = impl_validate_command(&ActionFuncsValidate::Name, action, tokens, types, indices); \
        if (!error.empty()) { mErrorLog.push_back(std::move(error)); return nullptr; } \
        return impl_bind_params(&ActionFuncs::Name, tokens, types, indices); \
    } while (false)

    if (tokens[0] == "enable_blob")    RETURN_BOUND_FUNCTION(enable_blob, PoolKey);
    if (tokens[0] == "disable_blob")   RETURN_BOUND_FUNCTION(disable_blob, PoolKey);
    if (tokens[0] == "add_velocity")   RETURN_BOUND_FUNCTION(add_velocity, float, float);
    if (tokens[0] == "finish_action")  RETURN_BOUND_FUNCTION(finish_action);
    if (tokens[0] == "emit_particles") RETURN_BOUND_FUNCTION(emit_particles, PoolKey, uint);

    #undef RETURN_BOUND_FUNCTION

    //--------------------------------------------------------//

    impl_log_error("unknown function '%s'", tokens[0]);

    return nullptr;
}

//============================================================================//

Vector<Action::Command> ActionBuilder::build_procedure(Action& action, StringView source)
{
    const auto sourceLines = sq::tokenise_string_view(source, '\n');

    if (sourceLines.empty() == true)
    {
        mErrorLog.emplace_back("source is empty");
    }

    Vector<Action::Command> result;
    result.reserve(sourceLines.size());

    for (const StringView& sourceLine : sourceLines)
    {
        auto boundFunc = build_command(action, sourceLine);
        if (boundFunc != nullptr) result.push_back(std::move(boundFunc));
    }

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

        procedure.commands = { build_command(action, procedure.meta.source) };

        action.rebuild_timeline();

        return;
    }

    //--------------------------------------------------------//

    const auto root = sq::parse_json_from_file(action.path);

    if (root.is_null() == true) return;

    //--------------------------------------------------------//

    try {
    for (auto iter : root.at("blobs").items())
    {
        HitBlob* blob = action.blobs.emplace(iter.key());

        blob->fighter = &action.fighter;
        blob->action = &action;

        try { blob->from_json(iter.value()); }
        catch (const std::exception& e) {
            impl_log_error("blob '%s': %s", iter.key(), e.what());
        }
    }
    } catch (const std::exception& e) { impl_log_error(e.what()); }

    //--------------------------------------------------------//

    try {
    for (auto iter : root.at("emitters").items())
    {
        ParticleEmitter* emitter = action.emitters.emplace(iter.key());

        emitter->fighter = &action.fighter;
        emitter->action = &action;

        try { emitter->from_json(iter.value()); }
        catch (const std::exception& e) {
            impl_log_error("emitter '%s': %s", iter.key(), e.what());
        }
    }
    } catch (const std::exception& e) { impl_log_error(e.what()); }

    //--------------------------------------------------------//

    try {
    for (auto iter : root.at("procedures").items())
    {
        Action::Procedure& procedure = action.procedures[iter.key()];

        try { iter.value().at("source").get_to(procedure.meta.source); }
        catch (const std::exception& e) {
            impl_log_error("procedure '%s': %s", iter.key(), e.what());
        }

        try { iter.value().at("frames").get_to(procedure.meta.frames); }
        catch (const std::exception& e) {
            impl_log_error("procedure '%s': %s", iter.key(), e.what());
        }

        procedure.commands = build_procedure(action, procedure.meta.source);
    }
    } catch (const std::exception& e) { impl_log_error(e.what()); }

    //--------------------------------------------------------//

    flush_logged_errors("errors in action '%s'"_fmt_(action.path));

    action.rebuild_timeline();
}

//============================================================================//

JsonValue ActionBuilder::serialise_as_json(const Action& action)
{
    JsonValue result;

    //--------------------------------------------------------//

    auto& resultEmitters = result["emitters"] = JsonValue::object();

    for (const auto& [key, emitter] : action.emitters)
        emitter->to_json(resultEmitters[key]);

    auto& resultBlobs = result["blobs"] = JsonValue::object();

    for (const auto& [key, blob] : action.blobs)
        blob->to_json(resultBlobs[key]);

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

//============================================================================//

void ActionBuilder::flush_logged_errors(String heading)
{
    if (mErrorLog.empty() == false)
    {
        sq::log_warning_block(heading, mErrorLog);
        mErrorLog.clear();
    }
}
