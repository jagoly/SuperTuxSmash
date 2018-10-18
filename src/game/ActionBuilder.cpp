#include <fstream> // for loading json

#include <sqee/helpers.hpp>
#include <sqee/misc/Parsing.hpp>

#include <sqee/debug/Logging.hpp>

#include <sqee/misc/Files.hpp>
#include <sqee/misc/Json.hpp>

#include "game/Actions.hpp"
#include "game/ActionFuncs.hpp"
#include "game/Fighter.hpp"

#include "game/ActionBuilder.hpp"

using json = nlohmann::json;
using sq::literals::operator""_fmt_;
using namespace sts;

//============================================================================//

namespace { // anonymous

//----------------------------------------------------------------------------//

using BoundFunction = std::function<void(Action& action)>;

thread_local Vector<String> gErrorLog;

template <class... Args>
void impl_log_error(const char* fmt, const Args&... args)
{ gErrorLog.emplace_back(tfm::format(fmt, args...)); }

//----------------------------------------------------------------------------//

template <class Type> Optional<Type> impl_convert_param(StringView token);

template <> Optional<int> impl_convert_param(StringView token) { return sq::safe_sv_to_i(token); }
template <> Optional<uint> impl_convert_param(StringView token) { return sq::safe_sv_to_u(token); }
template <> Optional<float> impl_convert_param(StringView token) { return sq::safe_sv_to_f(token); }

template <> Optional<PoolKey> impl_convert_param(StringView token)
{ return token.size() <= 15u ? std::make_optional<PoolKey>(token) : std::nullopt; }

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

    const Optional<Type> opt = impl_convert_param<Type>(token);
    if (!opt.has_value()) impl_log_error("invalid paramater %d: '%s'", Index, token);

    return opt.value();
}

template <class Func, class Types, size_t... Index> inline
BoundFunction impl_bind_params(Func func, const Vector<StringView>& tokens, Types, std::index_sequence<Index...>)
{
    if (tokens.size() != Types::Size + 1)
    {
        impl_log_error("wrong number of paramaters for command");
        return nullptr;
    }

    // this isn't a good use of exceptions, I'll refactor this later
    try { return std::bind(func, std::placeholders::_1, impl_extract_param<Types, Index>(tokens)...); }
    catch (std::bad_optional_access&) { return nullptr; }
}

//----------------------------------------------------------------------------//

} // anonymous namespace

//============================================================================//

std::function<void(Action& action)> ActionBuilder::build_command(Action& action, StringView source)
{
    const auto tokens = sq::tokenise_string_view(source, ' ');

    if (tokens.empty() == true)
    {
        impl_log_error("source is empty");
        return nullptr;
    }

    BoundFunction result;

    //--------------------------------------------------------//

    #define ASSIGN_BOUND_FUNCTION(Name, ...) result = impl_bind_params \
    ( &ActionFuncs::Name, tokens, TypePack<__VA_ARGS__ >(), std::index_sequence_for<__VA_ARGS__>() )

    if (tokens[0] == "enable_blob")    ASSIGN_BOUND_FUNCTION(enable_blob, PoolKey);
    if (tokens[0] == "disable_blob")   ASSIGN_BOUND_FUNCTION(disable_blob, PoolKey);
    if (tokens[0] == "add_velocity")   ASSIGN_BOUND_FUNCTION(add_velocity, float, float);
    if (tokens[0] == "finish_action")  ASSIGN_BOUND_FUNCTION(finish_action);
    if (tokens[0] == "emit_particles") ASSIGN_BOUND_FUNCTION(emit_particles, PoolKey, uint);

    #undef ASSIGN_BOUND_FUNCTION

    //--------------------------------------------------------//

    if (result == nullptr)
    {
        impl_log_error("unknown function: %s", tokens[0]);
        return nullptr;
    }

    //--------------------------------------------------------//

    return result;
}

//----------------------------------------------------------------------------//

void ActionBuilder::load_from_json(Action& action)
{
    action.blobs.clear(); // destroy existing blobs
    action.emitters.clear(); // destroy existing emitters
    action.commands.clear(); // clear existing commands

    // use a dummy action for actions I haven't written yet
    if (sq::check_file_exists(action.path) == false)
    {
        sq::log_warning("missing action '%s'", action.path);

        auto& command = action.commands.emplace_back();

        command.frames = { 12u };
        command.func = &ActionFuncs::finish_action;
        command.source = "finish_action";

        return;
    }

    //--------------------------------------------------------//

    const auto root = sq::parse_json_from_file(action.path);

    if (root.is_null() == true) return;

    //--------------------------------------------------------//

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

    //--------------------------------------------------------//

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

    //--------------------------------------------------------//

    for (const auto& item : root.at("commands"))
    {
        if (!item.is_array() || item.size() != 2u)
        {
            impl_log_error("invalid command: %s", item.dump());
            continue;
        }

        Action::Command& command = action.commands.emplace_back();

        try { item.front().get_to(command.frames); }
        catch (const std::exception& e) {
            impl_log_error("invalid command frames: %s", e.what());
        }

        command.source = item.back();

        command.func = ActionBuilder::build_command(action, command.source);
    }

    //--------------------------------------------------------//

    if (gErrorLog.empty() == false)
    {
        sq::log_warning_block("errors in action '%s'"_fmt_(action.path), gErrorLog);
        gErrorLog.clear();
    }
}

//----------------------------------------------------------------------------//

JsonValue ActionBuilder::serialise_as_json(const Action& action)
{
    JsonValue result;

    auto& resultEmitters = result["emitters"];
    auto& resultBlobs = result["blobs"];
    auto& resultCommands = result["commands"];

    //--------------------------------------------------------//

    for (auto& [key, emitter] : action.emitters)
    {
        emitter->to_json(resultEmitters[key]);
    }

    //--------------------------------------------------------//

    for (auto& [key, blob] : action.blobs)
    {
        blob->to_json(resultBlobs[key]);
    }

    //--------------------------------------------------------//

    for (const auto& command : action.commands)
    {
        resultCommands.emplace_back();

        resultCommands.back().push_back(command.frames);
        resultCommands.back().push_back(command.source);
    }

    return result;
}
