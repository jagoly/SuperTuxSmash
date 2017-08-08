#include <sqee/debug/Logging.hpp>

#include <sqee/misc/Files.hpp>
#include <sqee/misc/Json.hpp>

#include "game/FightSystem.hpp"
#include "game/Actions.hpp"

using json = nlohmann::json;
using namespace sts;

//============================================================================//

Action::Action(FightWorld& world, Fighter& fighter)
    : world(world), fighter(fighter)
    , blobs(world.get_hit_blob_allocator()) {}

Action::~Action() = default;

//============================================================================//

void Action::jump_to_frame(uint frame)
{
    mCurrentFrame = frame;
}

//============================================================================//

void Actions::load_json(const string& fighterName)
{
    const string basePath = "assets/fighters/" + fighterName + "/actions/";

    //--------------------------------------------------------//

    const auto load_single = [&](Action& action, const char* fileName)
    {
        // this is only temporary, all actions should have some json
        if (sq::check_file_exists(basePath + fileName) == false) return;

        const auto root = sq::parse_json(basePath + fileName);

        for (auto blobIter : json::iterator_wrapper(root.at("blobs")))
        {
            HitBlob* blob = action.blobs.emplace(blobIter.key().c_str());

            blob->fighter = &action.fighter;
            blob->action = &action;

            for (auto iter : json::iterator_wrapper(blobIter.value()))
            {
                const string key = iter.key();
                const auto& value = iter.value();

                if      (key == "origin")     blob->origin = { value[0], value[1], value[2] };
                else if (key == "radius")     blob->radius = value;
                else if (key == "bone")       blob->bone = value;
                else if (key == "group")      blob->group = value;
                else if (key == "knockAngle") blob->knockAngle = value;
                else if (key == "knockBase")  blob->knockBase = value;
                else if (key == "damage")     blob->damage = value;
                else if (key == "flavour")    blob->set_flavour_from_str(value);
                else if (key == "priority")   blob->set_priority_from_str(value);

                else sq::log_warning("unhandled blob key '%s'", key);
            }
        }
    };

    //--------------------------------------------------------//

    load_single ( *neutral_first, "Neutral_First.json" );
    load_single ( *tilt_down,     "Tilt_Down.json"     );
    load_single ( *tilt_forward,  "Tilt_Forward.json"  );
    load_single ( *tilt_up,       "Tilt_Up.json"       );
    load_single ( *air_back,      "Air_Back.json"      );
    load_single ( *air_down,      "Air_Down.json"      );
    load_single ( *air_forward,   "Air_Forward.json"   );
    load_single ( *air_neutral,   "Air_Neutral.json"   );
    load_single ( *air_up,        "Air_Up.json"        );
    load_single ( *dash_attack,   "Dash_Attack.json"   );
}

//============================================================================//

void Actions::tick_active_action()
{
    if (mActiveAction != nullptr)
    {
        if (mActiveAction->on_tick(mActiveAction->mCurrentFrame))
        {
            mActiveAction->on_finish();
            mActiveType = Action::Type::None;
            mActiveAction = nullptr;
        }
        else ++mActiveAction->mCurrentFrame;
    }
}

//============================================================================//

void DumbAction::on_start() { sq::log_info(" start: " + message); }

bool DumbAction::on_tick(uint frame) { return frame == 12u; }

void DumbAction::on_finish() { sq::log_info("finish: " + message); }

void DumbAction::on_collide(HitBlob*, Fighter&) {}
