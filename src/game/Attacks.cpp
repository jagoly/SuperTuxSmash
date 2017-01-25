#include <sqee/misc/Json.hpp>

#include "Attacks.hpp"

using namespace sts;
using Json = nlohmann::json;

//============================================================================//

Attack::Attack(AttackType type) : type(type) {}

Attack::~Attack() = default;

//============================================================================//

Attacks::Attacks(string name, Fighter& fighter) : mName(name), mFighter(fighter)
{

}

//============================================================================//

void Attacks::make_default_attacks()
{
    neutral_first = std::make_unique<Attack>(AttackType::Neutral_First);
    neutral_second = std::make_unique<Attack>(AttackType::Neutral_Second);
    neutral_third = std::make_unique<Attack>(AttackType::Neutral_Third);

    tilt_down = std::make_unique<Attack>(AttackType::Tilt_Down);
    tilt_forward = std::make_unique<Attack>(AttackType::Tilt_Forward);
    tilt_up = std::make_unique<Attack>(AttackType::Tilt_Up);
}

void Attacks::load_from_json()
{
    const string path = "assets/fighters/" + mName + "/attacks/";

    const auto load_attack = [&](unique_ptr<Attack>& attack, string str)
    {
        const auto json = sq::parse_json(path + str + ".json");

        attack->props.message = json.at("message");
        attack->props.time = json.at("time");
    };

    load_attack(neutral_first, "neutral_first");
    load_attack(neutral_second, "neutral_second");
    load_attack(neutral_third, "neutral_third");

    load_attack(tilt_down, "tilt_down");
    load_attack(tilt_forward, "tilt_forward");
    load_attack(tilt_up, "tilt_up");
}

//============================================================================//

void Attack::on_start()
{
    std::cout << " start: " << props.message << std::endl;
    mTimeLeft = 16u;
}

bool Attack::on_tick()
{
    //std::cout << "  tick: " << props.message << std::endl;
    return --mTimeLeft == 0u;
}

void Attack::on_finish()
{
    std::cout << "finish: " << props.message << std::endl;
}
