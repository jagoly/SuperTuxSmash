#include <fstream> // for loading json

#include <sqee/helpers.hpp>
#include <sqee/misc/Parsing.hpp>

#include <sqee/debug/Logging.hpp>

#include <sqee/misc/Files.hpp>
#include <sqee/misc/Json.hpp>

#include "game/Actions.hpp"
#include "game/ActionFuncs.hpp"

#include "game/ActionBuilder.hpp"

using json = nlohmann::json;
using sq::literals::operator""_fmt_;
using namespace sts;

//============================================================================//

namespace { // anonymous

//----------------------------------------------------------------------------//

using BoundFunction = std::function<void(Action& action)>;

thread_local std::vector<string> gErrorLog;

template <class... Args>
void impl_log_error(const char* fmt, const Args&... args)
{ gErrorLog.emplace_back(tfm::format(fmt, args...)); }

//----------------------------------------------------------------------------//

template <class Type> optional<Type> impl_convert_param(string_view token);

template <> optional<int> impl_convert_param(string_view token) { return sq::safe_sv_to_i(token); }
template <> optional<uint> impl_convert_param(string_view token) { return sq::safe_sv_to_u(token); }
template <> optional<float> impl_convert_param(string_view token) { return sq::safe_sv_to_f(token); }

template <> optional<PoolKey> impl_convert_param(string_view token)
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
auto impl_extract_param(const std::vector<string_view>& tokens)
{
    using Type = TypePackElement<Types, Index>;
    const string_view& token = tokens[Index + 1];

    const optional<Type> opt = impl_convert_param<Type>(token);
    if (!opt.has_value()) impl_log_error("invalid paramater %d: '%s'", Index, token);

    return opt.value();
}

template <class Func, class Types, size_t... Index> inline
BoundFunction impl_bind_params(Func func, const std::vector<string_view>& tokens, Types, std::index_sequence<Index...>)
{
    if (tokens.size() != Types::Size + 1)
    {
        impl_log_error("wrong number of paramaters for command");
        return BoundFunction();
    }

    // this isn't a good use of exceptions, I'll refactor this later
    try { return std::bind(func, std::placeholders::_1, impl_extract_param<Types, Index>(tokens)...); }
    catch (std::bad_optional_access) { return BoundFunction(); }
}

//----------------------------------------------------------------------------//

struct BlobTemplate
{
    uint8_t group;
    HitBlob::Flavour flavour;
    HitBlob::Priority priority;
    char _padding[1];
    float damage;
    float knockAngle;
    float knockBase;
    float knockScale;
};

//----------------------------------------------------------------------------//

} // anonymous namespace

//============================================================================//

optional<Action::Command> ActionBuilder::build_command(Action& action, string_view source)
{
    const auto tokens = sq::tokenise_string_view(source, ' ');

    if (tokens.empty() == true)
    {
        impl_log_error("source is empty");
        return std::nullopt;
    }

    //--------------------------------------------------------//

    Action::Command result;

    //--------------------------------------------------------//

    // todo: replace assertions with warnings so that I can easily
    //       edit actions at runtime and hotload them

    #define ASSIGN_BOUND_FUNCTION(Name, ...) result.func = impl_bind_params \
    ( &ActionFuncs::Name, tokens, TypePack<__VA_ARGS__ >(), std::index_sequence_for<__VA_ARGS__>() )

    if (tokens[0] == "enable_blob")   ASSIGN_BOUND_FUNCTION(enable_blob, PoolKey);
    if (tokens[0] == "disable_blob")  ASSIGN_BOUND_FUNCTION(disable_blob, PoolKey);
    if (tokens[0] == "add_velocity")  ASSIGN_BOUND_FUNCTION(add_velocity, float, float);
    if (tokens[0] == "finish_action") ASSIGN_BOUND_FUNCTION(finish_action);

    #undef ASSIGN_BOUND_FUNCTION

    //--------------------------------------------------------//

    if (result.func == nullptr)
    {
        impl_log_error("unknown function: %s", tokens[0]);
        return std::nullopt;
    }

    //--------------------------------------------------------//

    result.source = std::move(source);

    return result;
}

//----------------------------------------------------------------------------//

void ActionBuilder::load_from_json(Action& action)
{
    action.blobs.clear(); // destroy existing blobs
    action.timeline.clear(); // clear existing commands

    // use a dummy action for actions I haven't written yet
    if (sq::check_file_exists(action.path) == false)
    {
        sq::log_warning("missing action '%s'", action.path);

        action.timeline.push_back({12u, {}});
        auto& command = action.timeline.back().commands.emplace_back();

        command.source = "finish_action";
        command.func = &ActionFuncs::finish_action;

        return;
    }

    //--------------------------------------------------------//

    const auto root = sq::parse_json_from_file(action.path);

    if (root.is_null() == true) return;

    //--------------------------------------------------------//

    std::map<string, BlobTemplate> templates;

    //--------------------------------------------------------//

    if (root.count("templates") != 0u)
    for (auto iter : json::iterator_wrapper(root.at("templates")))
    {
        BlobTemplate& blobTemplate = templates[iter.key()];

        for (auto iter : json::iterator_wrapper(iter.value()))
        {
            const string& key = iter.key();
            const auto& value = iter.value();

            if      (key == "group")      blobTemplate.group = value;
            else if (key == "flavour")    blobTemplate.flavour = HitBlob::flavour_from_str(value);
            else if (key == "priority")   blobTemplate.priority = HitBlob::priority_from_str(value);
            else if (key == "damage")     blobTemplate.damage = value;
            else if (key == "knockAngle") blobTemplate.knockAngle = value;
            else if (key == "knockBase")  blobTemplate.knockBase = value;
            else if (key == "knockScale") blobTemplate.knockScale = value;

            else sq::log_warning("unhandled blob key '%s'", key);
        }
    }

    //--------------------------------------------------------//

    for (auto blobIter : json::iterator_wrapper(root.at("blobs")))
    {
        HitBlob* blob = action.blobs.emplace(blobIter.key().c_str());

        blob->fighter = &action.fighter;
        blob->action = &action;

        for (auto iter : json::iterator_wrapper(blobIter.value()))
        {
            const string key = iter.key();
            const auto& value = iter.value();

            if (key == "template")
            {
                const BlobTemplate& blobTemplate = templates.at(value);

                blob->group = blobTemplate.group;
                blob->flavour = blobTemplate.flavour;
                blob->priority = blobTemplate.priority;
                blob->damage = blobTemplate.damage;
                blob->knockAngle = blobTemplate.knockAngle;
                blob->knockBase = blobTemplate.knockBase;
                blob->knockScale = blobTemplate.knockScale;
            }

            else if (key == "origin")     blob->origin = { value[0], value[1], value[2] };
            else if (key == "radius")     blob->radius = value;
            else if (key == "bone")       blob->bone = value;
            else if (key == "group")      blob->group = value;
            else if (key == "knockAngle") blob->knockAngle = value;
            else if (key == "knockBase")  blob->knockBase = value;
            else if (key == "knockScale") blob->knockScale = value;
            else if (key == "damage")     blob->damage = value;
            else if (key == "flavour")    blob->set_flavour_from_str(value);
            else if (key == "priority")   blob->set_priority_from_str(value);

            else sq::log_warning("unhandled blob key '%s'", key);
        }
    }

    //--------------------------------------------------------//

    std::map<uint, std::vector<Action::Command>> frameMap;

    for (const auto& jsonTimeline : root.at("timeline"))
    {
        const auto& when = jsonTimeline[0];
        const auto& commandStrings = jsonTimeline[1];

        std::vector<Action::Command> commands;
        commands.reserve(commandStrings.size());

        for (const string& source : commandStrings)
        {
            auto command = ActionBuilder::build_command(action, source);
            if (command.has_value()) commands.push_back(std::move(*command));
            else impl_log_error("error building command: %s", source);
        }

        // todo: add some more sanity checking

        if (when.is_number_unsigned())
        {
            frameMap[when] = std::move(commands);
        }
        else if (when.is_string())
        {
            std::vector<uint> frames;

            const auto ranges = sq::tokenise_string_view(when.get_ref<const string&>(), ',');

            for (const auto& range : ranges)
            {
                SQASSERT(range.find_first_not_of("0123456789- ") == string::npos, "");

                if (const size_t sep = range.find('-'); sep != string::npos)
                {
                    const uint begin = sq::sv_to_u(range.substr(0u, sep));
                    const uint end = sq::sv_to_u(range.substr(sep + 1u));

                    for (uint i = begin; i < end; ++i)
                        frameMap[i] = commands;
                }
            }
        }
        else SQASSERT(false, "invalid frame or range");
    }

    // std::map is already ordered by keys
    for (const auto& item : frameMap)
    {
        action.timeline.push_back({item.first, std::move(item.second)});
    }

    //--------------------------------------------------------//

    if (gErrorLog.empty() == false)
    {
        sq::log_warning_block("errors in action '%s'"_fmt_(action.path), gErrorLog);
        gErrorLog.clear();
    }
}
